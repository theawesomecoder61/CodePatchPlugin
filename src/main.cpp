#include <cstdio>
#include <malloc.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>
#include <utility>

#include <coreinit/filesystem.h>
#include <coreinit/title.h>
#include <wups.h>
#include <wups/config/WUPSConfigItemBoolean.h>
#include <notifications/notifications.h>

#include "utils/logger.h"
#include "kernel.hpp"

WUPS_PLUGIN_NAME("Code Patch Plugin");
WUPS_PLUGIN_DESCRIPTION("This plugin dynamically patches executables with user-specified patches.");
WUPS_PLUGIN_VERSION("v1.2");
WUPS_PLUGIN_AUTHOR("pineapples721");
WUPS_PLUGIN_LICENSE("GPL");

#define ENABLED_CONFIG_ID "enabled"
#define NOTIFICATIONS_CONFIG_ID "notifications"
#define CODE_PATCH_PATH "fs:/vol/external01/wiiu/codepatches/"
#define FS_MAX_MOUNTPATH_SIZE 128

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE("codepatch");

bool enabled = true;
bool notifications = true;

int GetFileSize(const char *file) {
    struct stat fileStat;
    stat(file, &fileStat);
    return fileStat.st_size;
}

void Notify(const char *text) {
    if (!notifications) return;
    NotificationModuleHandle handle;
    NotificationModule_AddDynamicNotification(text, &handle);
    NotificationModule_FinishDynamicNotification(handle, 5);
}

bool ReadFile(const char *file, std::map<uint32_t, uint32_t> *patches) {
    uint32_t length = GetFileSize(file);
    // Ignoring the first two bytes, hax files are always multiples of 10
    if ((length - 2) % 10 != 0) {
        return false;
    }

    FILE *f = fopen(file, "r");

    uint16_t count = 0, size = 0;
    uint32_t addr = 0, code = 0;
    fread(&count, 2, 1, f);
    for (uint16_t i = 0; i < count; i++) {
        fread(&size, 2, 1, f);
        fread(&addr, 4, 1, f);
        fread(&code, 4, 1, f);
        // One patch per address
        if (patches->find(addr) == patches->end()) {
            patches->emplace(std::make_pair(addr, code));
        }
    }

    fclose(f);
    return true;
}

void ReadDir(const char *path, std::map<uint32_t, uint32_t> *patches) {
    DIR *dir = opendir(path);
    struct dirent *dp;
    if (dir == NULL) {
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strstr(dp->d_name, ".hax") != NULL) {
            char file[128];
            strcpy(file, path);
            strcat(file, dp->d_name);
            if (ReadFile(file, patches)) {
                char msg[280];
                sprintf(msg, "Loaded \"%s\"", dp->d_name);
                Notify(msg);
            }
        }
    }

    closedir(dir);
}

void ApplyPatches(std::map<uint32_t, uint32_t> patches) {
    for (const auto &patch : patches) {
        KernelWriteU32(patch.first, patch.second);
    }
}

bool FolderExists(const char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        return true;
    } else {
        return false;
    }
}

bool IsSystemTitle(uint64_t titleId) {
    uint32_t id = titleId >> 32;
    return id == 0x00050010 || id == 0x0005001B || id == 0x00050030 || id == 0x0005000E;
}

void InitModules() {
    NotificationModule_InitLibrary();
}

void DeinitModules() {
    NotificationModule_DeInitLibrary();
}

INITIALIZE_PLUGIN() {
    // Logging only works when compiled with `make DEBUG=1`
    initLogging();
    InitModules();

    // Open storage to read values
    WUPSStorageError storageRes = WUPS_OpenStorage();
    if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
    } else {
        // Try to get values from storage
        if ((storageRes = WUPS_GetBool(nullptr, ENABLED_CONFIG_ID, &enabled)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it is missing
            if (WUPS_StoreBool(nullptr, ENABLED_CONFIG_ID, enabled) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE("Failed to store bool");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        if ((storageRes = WUPS_GetBool(nullptr, NOTIFICATIONS_CONFIG_ID, &notifications)) == WUPS_STORAGE_ERROR_NOT_FOUND) {
            // Add the value to the storage if it is missing
            if (WUPS_StoreBool(nullptr, NOTIFICATIONS_CONFIG_ID, notifications) != WUPS_STORAGE_ERROR_SUCCESS) {
                DEBUG_FUNCTION_LINE("Failed to store bool");
            }
        } else if (storageRes != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to get bool %s (%d)", WUPS_GetStorageStatusStr(storageRes), storageRes);
        }

        // Close storage
        if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
            DEBUG_FUNCTION_LINE("Failed to close storage");
        }
    }

    deinitLogging();
}

DEINITIALIZE_PLUGIN() {
    DeinitModules();
}

ON_APPLICATION_START() {
    initLogging();

    uint64_t titleId = OSGetTitleID();
    char path[FS_MAX_MOUNTPATH_SIZE];
    std::map<uint32_t, uint32_t> patches;

    if (!enabled || IsSystemTitle(titleId)) {
        goto quit;
    }

    if (!FolderExists(CODE_PATCH_PATH)) {
        DEBUG_FUNCTION_LINE("Patches folder is missing");
        goto quit;
    }

    sprintf(path, "%s%016llX/", CODE_PATCH_PATH, titleId);
    if (!FolderExists(path)) {
        DEBUG_FUNCTION_LINE("No patch folder for this title");
        Notify("No patches for this title");
        goto quit;
    }

    DEBUG_FUNCTION_LINE("Trying to patch %lld", titleId);

    ReadDir(path, &patches);
    DEBUG_FUNCTION_LINE("Read patches for %lld", titleId);

    if (patches.size() > 0) {
        ApplyPatches(patches);
        DEBUG_FUNCTION_LINE("Applied patches to %lld", titleId);
        Notify("Applied patches");
    } else {
        DEBUG_FUNCTION_LINE("No patches for %lld", titleId);
    }

    quit:
        deinitLogging();
}

ON_APPLICATION_ENDS() {
}

ON_APPLICATION_REQUESTS_EXIT() {
}

void enabledChanged(ConfigItemBoolean *item, bool newValue) {
    enabled = newValue;
    WUPS_StoreInt(nullptr, ENABLED_CONFIG_ID, enabled);
}

void notificationsChanged(ConfigItemBoolean *item, bool newValue) {
    notifications = newValue;
    WUPS_StoreInt(nullptr, NOTIFICATIONS_CONFIG_ID, notifications);
}

WUPS_GET_CONFIG() {
    // Open the storage, so we can persist the configuration the user made
    if (WUPS_OpenStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE("Failed to open storage");
        return 0;
    }

    WUPSConfigHandle config;
    WUPSConfig_CreateHandled(&config, "Code Patch Plugin");

    WUPSConfigCategoryHandle cat;
    WUPSConfig_AddCategoryByNameHandled(config, "Settings", &cat);

    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, ENABLED_CONFIG_ID, "Enabled (effective next application launch)", enabled, &enabledChanged);
    WUPSConfigItemBoolean_AddToCategoryHandled(config, cat, NOTIFICATIONS_CONFIG_ID, "Notifications", notifications, &notificationsChanged);

    return config;
}

WUPS_CONFIG_CLOSED() {
    // Save changes
    if (WUPS_CloseStorage() != WUPS_STORAGE_ERROR_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to close storage");
    }
}