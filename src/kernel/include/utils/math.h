#pragma once
#include "klib/stdbool.h"
#include "klib/stdint.h"
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

extern void __attribute__((cdecl))
div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient_out,
         uint32_t *reminder_out);

static inline uintptr_t align_down(uintptr_t num, uintptr_t jump_size) {
  return num & ~(jump_size - 1);
}

static inline uintptr_t align_up(uintptr_t num, uintptr_t jump_size) {
  return align_down(num + jump_size - 1, jump_size);
}
static inline bool is_aligned(uintptr_t num, uintptr_t jump_size) {
  return (num & (jump_size - 1)) == 0;
}
