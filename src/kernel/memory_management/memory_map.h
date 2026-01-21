#pragma once
#include "utils/range.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t start;
  uint32_t size;
  uint32_t type;
} MemoryRegion;

typedef struct {
  uint32_t region_count;
  MemoryRegion *regions;
} MemoryMap;

#define MAX_USED_RANGES 64

typedef struct {
  Range ranges[MAX_USED_RANGES];
  size_t count;
} RangeList;

static void range_list_push(RangeList *list, Range r) {
  if (list->count < MAX_USED_RANGES) {
    list->ranges[list->count++] = r;
  }
}

Range get_memory_range(MemoryMap *memory_map);

void collect_non_usable_ranges(RangeList *list, MemoryMap *memory_map,
                               Range memory_range);
