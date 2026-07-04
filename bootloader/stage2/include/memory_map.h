/**
 * @file memory_map.h
 * @brief E820 memory map detection.
 */
#pragma once

#include "boot/bootparams.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint64_t base, length;
  uint32_t type, acpi_flag, reserved1, reserved2;
} __attribute__((packed)) MemoryRegionInternal;

typedef struct {
  int region_count;
  MemoryRegionInternal *regions;
} MemoryMapInternal;

int __attribute__((cdecl))
x86_e820_get_next_block(MemoryRegionInternal *block, uint32_t *continuationId);

void memory_map_detect(MemoryMapInternal *memory_map);
