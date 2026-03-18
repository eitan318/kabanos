#include "klib/stdbool.h"
#include "klib/stdint.h"

static inline void bitmap_set(uint8_t *bitmap, uint64_t bit) {
  bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint8_t *bitmap, uint64_t bit) {
  bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline bool bitmap_test(uint8_t *bitmap, uint64_t bit) {
  return bitmap[bit / 8] & (1 << (bit % 8));
}

// Set all bits in range [start, end)
static inline void bitmap_set_range(uint8_t *bitmap, uint64_t start,
                                    uint64_t end) {
  for (uint64_t i = start; i < end; i++) {
    bitmap_set(bitmap, i);
  }
}

// Clear all bits in range [start, end)
static inline void bitmap_clear_range(uint8_t *bitmap, uint64_t start,
                                      uint64_t end) {
  for (uint64_t i = start; i < end; i++) {
    bitmap_clear(bitmap, i);
  }
}

// Find first clear bit, returns UINT64_MAX if none found
static inline uint64_t bitmap_find_first_clear(uint8_t *bitmap, uint64_t size) {
  for (uint64_t i = 0; i < size; i++) {
    if (!bitmap_test(bitmap, i)) {
      return i;
    }
  }
  return UINT64_MAX;
}

// Fill entire bitmap with a value
static inline void bitmap_fill(uint8_t *bitmap, uint64_t size_bytes,
                               uint8_t value) {
  for (uint64_t i = 0; i < size_bytes; i++) {
    bitmap[i] = value;
  }
}
