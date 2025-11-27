#pragma once

#include "boot/bootparams.h"
#include <stdbool.h>
#include <stdint.h>

enum E820MemoryBlockType {
  E820_USABLE = 1,
  E820_RESERVED = 2,
  E820_ACPI_RECLAIMABLE = 3,
  E820_ACPI_NVS = 4,
  E820_BAD_MEMORY = 5,
};

int __attribute__((cdecl))
x86_e820_get_next_block(MemoryRegion *block, uint32_t *continuationId);

void memory_map_detect(MemoryMap *memory_map);
