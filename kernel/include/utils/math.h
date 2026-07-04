/**
 * @file math.h
 * @brief Min/max and power-of-two alignment helpers.
 */
#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

/** @brief Rounds @p num down to a multiple of @p jump_size (a power of two). */
static inline uintptr_t align_down(uintptr_t num, uintptr_t jump_size) {
  return num & ~(jump_size - 1);
}

/** @brief Rounds @p num up to a multiple of @p jump_size (a power of two). */
static inline uintptr_t align_up(uintptr_t num, uintptr_t jump_size) {
  return align_down(num + jump_size - 1, jump_size);
}

/** @brief True if @p num is a multiple of @p jump_size (a power of two). */
static inline bool is_aligned(uintptr_t num, uintptr_t jump_size) {
  return (num & (jump_size - 1)) == 0;
}
