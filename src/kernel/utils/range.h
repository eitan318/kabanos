#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct Range {
  uintptr_t start;
  uintptr_t end;
} Range;

Range range_clamp(Range src, Range boundry);
Range range_align_outward(Range range, uint64_t block_size);
Range range_align_inward(Range range, uint64_t block_size);
bool in_range(uint64_t addr, Range range);
