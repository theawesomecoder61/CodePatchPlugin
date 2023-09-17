// https://github.com/wiiu-env/EnvironmentLoader
#pragma once
#include <coreinit/cache.h>
#include <coreinit/memorymap.h>
#include <whb/log.h>
#include <whb/log_udp.h>
#include <wums.h>

extern "C" void SCKernelCopyData(uint32_t dst, uint32_t src, uint32_t len);
extern "C" void SC_KernelCopyData(uint32_t addr, uint32_t src, uint32_t len);

#define KERN_SYSCALL_TBL_1 0xFFE84C70 //Unknown
#define KERN_SYSCALL_TBL_2 0xFFE85070 //Games
#define KERN_SYSCALL_TBL_3 0xFFE85470 //Loader
#define KERN_SYSCALL_TBL_4 0xFFEAAA60 //Home menu
#define KERN_SYSCALL_TBL_5 0xFFEAAE60 //Browser

/* Write a 32-bit word with kernel permissions */
void __attribute__((noinline)) kern_write(void *addr, uint32_t value) {
    asm volatile(
            "li 3,1\n"
            "li 4,0\n"
            "mr 5,%1\n"
            "li 6,0\n"
            "li 7,0\n"
            "lis 8,1\n"
            "mr 9,%0\n"
            "mr %1,1\n"
            "li 0,0x3500\n"
            "sc\n"
            "nop\n"
            "mr 1,%1\n"
            :
            : "r"(addr), "r"(value)
            : "memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
              "11", "12");
}

// https://github.com/wiiu-env/payload_loader
/* Read a 32-bit word with kernel permissions */
uint32_t __attribute__ ((noinline)) kern_read(const void *addr) {
    uint32_t result;
    asm volatile (
        "li 3,1\n"
        "li 4,0\n"
        "li 5,0\n"
        "li 6,0\n"
        "li 7,0\n"
        "lis 8,1\n"
        "mr 9,%1\n"
        "li 0,0x3400\n"
        "mr %0,1\n"
        "sc\n"
        "nop\n"
        "mr 1,%0\n"
        "mr %0,3\n"
        :	"=r"(result)
        :	"b"(addr)
        :	"memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
        "11", "12"
    );

    return result;
}

void KernelWriteU32(uint32_t addr, uint32_t value) {
    kern_write((void *) (KERN_SYSCALL_TBL_2 + (0x25 * 4)), (unsigned int) SCKernelCopyData);

    ICInvalidateRange(&value, 4);
    DCFlushRange(&value, 4);

    auto dst = (uint32_t) OSEffectiveToPhysical(addr);
    auto src = (uint32_t) OSEffectiveToPhysical((uint32_t) &value);

    SC_KernelCopyData(dst, src, 4);

    DCFlushRange((void *) addr, 4);
    ICInvalidateRange((void *) addr, 4);
}

// uint32_t KernelReadU32(uint32_t addr) {
//     uint32_t value = kern_read((void *) OSEffectiveToPhysical(addr));
//     return value;
// }