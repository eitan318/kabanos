#pragma once

#include "adt/range.h"

#define FRAME_SIZE 4096

typedef uint32_t paddr_t;

void pmm_init(range_t total_range, range_t *usable_ranges,
              int usable_ranges_count, range_t *critical_ranges,
              int critical_ranges_count);

uint64_t pmm_get_metadata_size(range_t total_memory_range);
uint64_t pmm_frame_alloc();
void pmm_frame_free(paddr_t frame_addr);
void pmm_frame_refcount_inc(paddr_t frame_addr);
uint16_t pmm_frame_refcount_get(paddr_t frame_addr);

uint64_t frame_get_free_count();
uint64_t frame_get_used_count();
uint64_t frame_get_total_count();

// for unit test
void pmm_mark_range_used(paddr_t from, paddr_t to);
