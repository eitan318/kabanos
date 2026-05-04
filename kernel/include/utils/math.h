#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

static inline uintptr_t align_down(uintptr_t num, uintptr_t jump_size) {
  return num & ~(jump_size - 1);
}

static inline uintptr_t align_up(uintptr_t num, uintptr_t jump_size) {
  return align_down(num + jump_size - 1, jump_size);
}
static inline bool is_aligned(uintptr_t num, uintptr_t jump_size) {
  return (num & (jump_size - 1)) == 0;
}
