# Code Patch Plugin
This plugin loads user-provided code patches (stored in the [hax format](#hax-files)) and dynamically applies them to the current application. It supports WUPS and Aroma (alternative environment to Tiramisu) and was inspired by [CafeLoader](https://github.com/aboood40091/CafeLoader). The application's executable file remains untouched by the plugin.

Code Patch will load all `.hax` files in a folder and apply all patches found therein. Unlike CafeLoader, you do not need to compile a project in order to create patches.

*Currently, only `.hax` files are supported. The addr/code `.bin` files and network loading features that CafeLoader has may be included in a future version.*

---

**Note**: New/additional instructions or routines are not supported. Only instruction replacements are allowed.

**Note**: Code Patch will not run on system applications or applets.

---

## Installation
Be sure you fully installed and configured Tiramisu and Aroma on your Wii U.

1. Copy `CodePatchPlugin.wps` into `/fs/vol/external01/wiiu/environments/aroma/plugins`.
2. Create a folder in `/fs/vol/external01/wiiu/` called `codepatches`.

## Usage
For each application you wish to apply patches, create a folder with the base title ID in the path `/fs/vol/external01/wiiu/codepatches/[title ID]`, where `[title ID]` is the base title ID, and place `.hax` files in there.

Patching can be enabled/disabled globally via the WUPS config menu (press L, DPAD Down and Minus on the GamePad, Pro Controller or Classic Controller).

When loading and applying patches, the plugin will display messages in the top left. These too can be enabled/disabled in the WUPS config menu.

I tested on a 32 GB US model running 5.5.5. This plugin will work on all 5.5.X versions that Tiramisu supports.

## hax files
A `.hax` file is a format that contains individual code replacements with corresponding addresses. Since the Wii U's CPU is big-endian, `.hax` files must also be. I did not invent this format; I use it since CafeLoader used it.

For reference, here is a breakdown of the format.

**Offset**|**Type**|**Description**
:-----:|:-----:|:-----:
0|uint16|number of patches
2|Patch|patches

#### Patch
**Offset**|**Type**|**Description**
:-----:|:-----:|:-----:
0|uint16|size of code (should be 4)
2|uint32|address (for BotW, this value minus 0xA900000 = original address)
6|uint32|code

### Python script
I included a Python script that converts an `.txt` file to a `.hax` file. Using this is not required. Before running the script, install `keystone` from pip.

Run the script like so: `python txt_to_hax.py [txt file]`. The resulting `.hax` file will appear in the same directory with the same name as the `.hax` file.

The input file must have patches in this format, seperated by newlines. Any line not in this format will be ignored. Lines beginning with `;` are treated as comments and are also ignored.
```
[address in hex starting with 0x] = [PPC instruction]
```

## Building
To build, you need:

- [Wii U Plugin System](https://github.com/Maschell/WiiUPluginSystem)
- [Wii U Module System](https://github.com/wiiu-env/WiiUModuleSystem)
- [NotificationModule](https://github.com/wiiu-env/NotificationModule)
- [wut](https://github.com/devkitpro/wut)

Install them with their dependencies in this order according to their READMEs. After, compile the plugin using `make` (with no logging) or `make DEBUG=1` (with logging).

## Buildflags

### Logging
Building via `make` only logs errors (via OSReport). To enable logging via the [LoggingModule](https://github.com/wiiu-env/LoggingModule) set `DEBUG` to `1` or `VERBOSE`.

`make` Logs errors only (via OSReport).  
`make DEBUG=1` Enables information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).  
`make DEBUG=VERBOSE` Enables verbose information and error logging via [LoggingModule](https://github.com/wiiu-env/LoggingModule).

If the [LoggingModule](https://github.com/wiiu-env/LoggingModule) is not present, it will fallback to UDP (port 4405) and [CafeOS](https://github.com/wiiu-env/USBSerialLoggingModule) logging. You can use `udplogserver` (/opt/devkitpro/tools/bin/udplogserver) to view logs.