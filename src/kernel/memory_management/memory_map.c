#include "memory_map.h"
#include "boot/bootparams.h"
#include "memory_management/pmm.h"
#include "string.h"

void collect_non_usable_ranges(RangeList *list, MemoryMap *memory_map,
                               Range memory_range) {
  for (int i = 0; i < memory_map->region_count; i++) {
    MemoryRegion *region = &memory_map->regions[i];

    if (region->type == E820_USABLE)
      continue;

    Range r = {.start = region->start, .end = region->start + region->size};
    r = range_align_outward(r, FRAME_SIZE);
    r = range_clamp(r, memory_range);

    if (r.start < r.end)
      range_list_push(list, r);
  }
}

Range get_memory_range(MemoryMap *memory_map) {
  uintptr_t max_addr = 0;          // Initialize to 0
  uintptr_t min_addr = UINT64_MAX; // Initialize to max value

  for (int i = 0; i < memory_map->region_count; i++) {
    MemoryRegion *region = &memory_map->regions[i];

    if (region->type != E820_USABLE)
      continue;

    uintptr_t region_end = region->start + region->size;
    if (region->start < min_addr)
      min_addr = region->start;
    if (region_end > max_addr)
      max_addr = region_end;
  }

  Range range = {
      .start = min_addr,
      .end = max_addr,
  };

  return range;
}
