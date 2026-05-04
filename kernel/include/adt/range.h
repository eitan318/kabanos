#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

typedef struct range {
  uintptr_t start;
  uintptr_t end;
} range_t;

range_t range_clamp(range_t src, range_t boundry);
range_t range_align_outward(range_t range, uint64_t block_size);
range_t range_align_inward(range_t range, uint64_t block_size);
bool in_range(uint64_t addr, range_t range);
