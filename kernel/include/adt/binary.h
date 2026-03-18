#pragma once
#include "klib/stdint.h"

/* Mask operations */
#define MASK_SET(buf, flag_mask) ((buf) |= (flag_mask))
#define MASK_UNSET(buf, flag_mask) ((buf) &= ~(flag_mask))
#define MASK_CHECK(buf, flag_mask) (((buf) & (flag_mask)) != 0)

/* Get bits [from..to] from a value */
static inline uint32_t GET_BITS(uint32_t val, unsigned from, unsigned count) {
  return (val >> from) & ((1U << (count + 1)) - 1);
}
