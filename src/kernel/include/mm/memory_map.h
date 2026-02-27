#pragma once
#include "adt/range.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

enum E820MemoryBlockType {
  E820_USABLE = 1,
  E820_RESERVED = 2,
  E820_ACPI_RECLAIMABLE = 3,
  E820_ACPI_NVS = 4,
  E820_BAD_MEMORY = 5,
};

typedef struct {
  uint64_t start;
  uint64_t size;
  uint32_t type;
} memory_region_t;

typedef struct {
  uint32_t region_count;
  memory_region_t *regions;
} memory_map_t;

#define MAX_USED_RANGES 64

typedef struct {
  Range ranges[MAX_USED_RANGES];
  size_t count;
} range_list_t;

static void range_list_push(range_list_t *list, Range r) {
  if (list->count < MAX_USED_RANGES) {
    list->ranges[list->count++] = r;
  }
}

Range get_memory_range(memory_map_t *memory_map);

void collect_non_usable_ranges(range_list_t *list, memory_map_t *memory_map,
                               Range memory_range);
