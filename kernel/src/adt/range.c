/**
 * @file range.c
 * @brief Address-range clamping and alignment.
 */
#include "adt/range.h"
#include "utils/math.h"

range_t range_clamp(range_t src, range_t boundry) {
  range_t clamped = {
      MAX(src.start, boundry.start),
      MIN(src.end, boundry.end),
  };
  return clamped;
}

range_t range_align_outward(range_t range, uint64_t block_size) {
  range_t aligned = {
      .start = align_down(range.start, block_size),
      .end = align_up(range.end, block_size),
  };
  return aligned;
}

range_t range_align_inward(range_t range, uint64_t block_size) {
  range_t aligned = {
      .start = align_up(range.start, block_size),
      .end = align_down(range.end, block_size),
  };
  return aligned;
}

bool in_range(uint64_t addr, range_t range) {
  return addr >= range.start && addr < range.end;
}
