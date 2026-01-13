#pragma once

#include "boot/bootparams.h"
#include "utils/range.h"
#include <stdbool.h>
#include <stdint.h>

#define FRAME_SIZE 4096

typedef uint32_t paddr_t;

void pmm_init(Range memory_range, Range *used_ranges, int used_ranges_count);
uint64_t pmm_frame_alloc();
void pmm_frame_free(uint64_t frame_addr);
uint64_t frame_get_free_count();
uint64_t frame_get_used_count();
uint64_t frame_get_total_count();

// for unit test
void pmm_mark_range_used(paddr_t from, paddr_t to);
