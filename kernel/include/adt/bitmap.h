/**
 * @file bitmap.h
 * @brief Simple byte-array bitmap primitives.
 */
#include "klib/stdbool.h"
#include "klib/stdint.h"

/** @brief Sets bit @p bit. */
static inline void bitmap_set(uint8_t *bitmap, uint64_t bit) {
  bitmap[bit / 8] |= (1 << (bit % 8));
}

/** @brief Clears bit @p bit. */
static inline void bitmap_clear(uint8_t *bitmap, uint64_t bit) {
  bitmap[bit / 8] &= ~(1 << (bit % 8));
}

/** @brief Returns true if bit @p bit is set. */
static inline bool bitmap_test(uint8_t *bitmap, uint64_t bit) {
  return bitmap[bit / 8] & (1 << (bit % 8));
}

/** @brief Sets all bits in the range [start, end). */
static inline void bitmap_set_range(uint8_t *bitmap, uint64_t start,
                                    uint64_t end) {
  for (uint64_t i = start; i < end; i++) {
    bitmap_set(bitmap, i);
  }
}

/** @brief Clears all bits in the range [start, end). */
static inline void bitmap_clear_range(uint8_t *bitmap, uint64_t start,
                                      uint64_t end) {
  for (uint64_t i = start; i < end; i++) {
    bitmap_clear(bitmap, i);
  }
}

/**
 * @brief Finds the first clear bit.
 * @param size Number of bits to scan.
 * @return Bit index, or UINT64_MAX if every bit is set.
 */
static inline uint64_t bitmap_find_first_clear(uint8_t *bitmap, uint64_t size) {
  for (uint64_t i = 0; i < size; i++) {
    if (!bitmap_test(bitmap, i)) {
      return i;
    }
  }
  return UINT64_MAX;
}

/** @brief Fills the whole bitmap buffer with the byte @p value. */
static inline void bitmap_fill(uint8_t *bitmap, uint64_t size_bytes,
                               uint8_t value) {
  for (uint64_t i = 0; i < size_bytes; i++) {
    bitmap[i] = value;
  }
}
