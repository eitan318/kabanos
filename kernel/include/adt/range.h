/**
 * @file range.h
 * @brief Address-range type and alignment helpers.
 */
#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

/** @brief Half-open address range [start, end). */
typedef struct range {
  uintptr_t start;
  uintptr_t end;
} range_t;

/** @brief Intersects @p src with @p boundry. */
range_t range_clamp(range_t src, range_t boundry);

/** @brief Grows the range so both ends are @p block_size aligned. */
range_t range_align_outward(range_t range, uint64_t block_size);

/** @brief Shrinks the range so both ends are @p block_size aligned. */
range_t range_align_inward(range_t range, uint64_t block_size);

/** @brief True if @p addr lies inside @p range. */
bool in_range(uint64_t addr, range_t range);
