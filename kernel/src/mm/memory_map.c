/**
 * @file memory_map.c
 * @brief Queries over the E820-style physical memory map.
 */
#include "mm/memory_map.h"

range_t get_memory_range(memory_map_t *memory_map) {
  uintptr_t max_addr = 0;
  uintptr_t min_addr = UINT64_MAX;

  for (int i = 0; i < memory_map->region_count; i++) {
    memory_region_t *region = &memory_map->regions[i];

    if (region->type != E820_USABLE)
      continue;

    uintptr_t region_end = region->start + region->size;
    if (region->start < min_addr)
      min_addr = region->start;
    if (region_end > max_addr)
      max_addr = region_end;
  }

  range_t range = {
      .start = min_addr,
      .end = max_addr,
  };

  return range;
}
