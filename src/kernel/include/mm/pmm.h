#pragma once

#include "adt/range.h"
#include <stdbool.h>
#include <stdint.h>

#define FRAME_SIZE 4096

typedef uint32_t paddr_t;

void pmm_init(Range memory_range, Range *used_ranges, int used_ranges_count);

uint64_t pmm_get_metadata_size(Range total_memory_range);

// Allocates a frame and sets its refcount to 1.
uint64_t pmm_frame_alloc();

// Decrements refcount. If refcount becomes 0, marks frame as free in bitmap.
void pmm_frame_free(paddr_t frame_addr);

// Increments refcount. Used during fork() when a child inherits a page.
void pmm_frame_refcount_inc(paddr_t frame_addr);

/**
 * Returns the current number of owners for a frame.
 * Used by the COW handler to decide if it needs to copy or can just re-map.
 */
uint16_t pmm_frame_refcount_get(paddr_t frame_addr);

uint64_t frame_get_free_count();
uint64_t frame_get_used_count();
uint64_t frame_get_total_count();

// for unit test
void pmm_mark_range_used(paddr_t from, paddr_t to);
