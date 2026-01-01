#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct Range {
  uint64_t start;
  uint64_t end;
} Range;

Range range_clamp(Range src, Range boundry);
Range range_align_outward(Range range, uint64_t block_size);
bool in_range(uint64_t addr, Range range);
