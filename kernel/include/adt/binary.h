/**
 * @file binary.h
 * @brief Bit-mask and bit-field helpers.
 */
#pragma once
#include "klib/stdint.h"

/* Mask operations */
#define MASK_SET(buf, flag_mask) ((buf) |= (flag_mask))
#define MASK_UNSET(buf, flag_mask) ((buf) &= ~(flag_mask))
#define MASK_CHECK(buf, flag_mask) (((buf) & (flag_mask)) != 0)

/**
 * @brief Extracts @p count + 1 bits of @p val starting at bit @p from.
 *
 * Note the off-by-one convention: passing count=3 extracts 4 bits.
 */
static inline uint32_t GET_BITS(uint32_t val, unsigned from, unsigned count) {
  return (val >> from) & ((1U << (count + 1)) - 1);
}
