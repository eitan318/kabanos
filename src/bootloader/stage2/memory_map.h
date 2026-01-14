#pragma once

#include "boot/bootparams.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t base, length;
  uint32_t type, acpi_flag, reserved1, reserved2;
} __attribute__((packed)) MemoryRegion;

typedef struct {
  int region_count;
  MemoryRegion *regions;
} MemoryMap;

int __attribute__((cdecl))
x86_e820_get_next_block(MemoryRegion *block, uint32_t *continuationId);

void memory_map_detect(MemoryMap *memory_map);
