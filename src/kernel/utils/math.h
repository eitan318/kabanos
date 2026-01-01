#pragma once
#include <stdint.h>
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

extern void __attribute__((cdecl))
div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotient_out,
         uint32_t *reminder_out);

static inline uint32_t align_down(uint32_t num, uint32_t jump_size) {
  return num & ~(jump_size - 1);
}

static inline uint32_t align_up(uint32_t num, uint32_t jump_size) {
  return align_down(num + jump_size - 1, num);
}
