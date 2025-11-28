#pragma once

#include "boot/bootparams.h"
#include <stdbool.h>
#include <stdint.h>

int __attribute__((cdecl))
x86_e820_get_next_block(MemoryRegion *block, uint32_t *continuationId);

void memory_map_detect(MemoryMap *memory_map);
