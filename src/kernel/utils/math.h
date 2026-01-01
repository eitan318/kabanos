#pragma once
#include <stdint.h>
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

extern void __attribute__((cdecl))
div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient_out,
         uint32_t *reminder_out);

static inline uint64_t align_down(uint64_t num, uint64_t jump_size) {
  return num & ~(jump_size - 1);
}

static inline uint64_t align_up(uint64_t num, uint64_t jump_size) {
  return align_down(num + jump_size - 1, jump_size);
}
