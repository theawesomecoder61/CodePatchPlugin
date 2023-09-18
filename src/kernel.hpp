#pragma once
#include <coreinit/cache.h>
#include <coreinit/memorymap.h>
#include <wums.h>
#include <kernel/kernel.h>

void KernelWriteU32(uint32_t addr, uint32_t value) {
    ICInvalidateRange(&value, 4);
    DCFlushRange(&value, 4);

    auto dst = (uint32_t) OSEffectiveToPhysical(addr);
    auto src = (uint32_t) OSEffectiveToPhysical((uint32_t) &value);

    KernelCopyData(dst, src, 4);

    DCFlushRange((void *) addr, 4);
    ICInvalidateRange((void *) addr, 4);
}