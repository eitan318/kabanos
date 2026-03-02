#include "adt/range.h"
#include "utils/math.h"

Range range_clamp(Range src, Range boundry) {
  Range clamped = {
      MAX(src.start, boundry.start),
      MIN(src.end, boundry.end),
  };
  return clamped;
}

Range range_align_outward(Range range, uint64_t block_size) {
  Range aligned = {
      .start = align_down(range.start, block_size),
      .end = align_up(range.end, block_size),
  };
  return aligned;
}

Range range_align_inward(Range range, uint64_t block_size) {
  Range aligned = {
      .start = align_up(range.start, block_size),
      .end = align_down(range.end, block_size),
  };
  return aligned;
}

bool in_range(uint64_t addr, Range range) {
  return addr >= range.start && addr < range.end;
}
