#include "memory.h"

void *memcpy(void *dst, const void *src, uint32_t num) {
  void *ret = dst;
  uint32_t dwords = num >> 2;
  uint32_t rem = num & 3;

  __asm__ volatile("rep movsl"
                   : "+D"(dst), "+S"(src), "+c"(dwords)
                   :
                   : "memory");
  // Remaining bytes
  __asm__ volatile("rep movsb" : "+D"(dst), "+S"(src), "+c"(rem) : : "memory");
  return ret;
}

void *memset(void *ptr, int value, uint32_t num) {
  void *ret = ptr;
  uint8_t byte = (uint8_t)value;
  uint32_t pattern = byte | ((uint32_t)byte << 8) | ((uint32_t)byte << 16) |
                     ((uint32_t)byte << 24);
  uint32_t dwords = num >> 2;
  uint32_t rem = num & 3;
  uint8_t *dest = (uint8_t *)ptr;

  __asm__ volatile("rep stosl"
                   : "+D"(dest), "+c"(dwords)
                   : "a"(pattern)
                   : "memory");
  while (rem--)
    *dest++ = byte;
  return ret;
}

int memcmp(const void *ptr1, const void *ptr2, uint32_t num) {
  const uint8_t *p1 = (const uint8_t *)ptr1;
  const uint8_t *p2 = (const uint8_t *)ptr2;
  // repe cmpsb compares byte-by-byte and stops at first mismatch
  uint32_t count = num;

  __asm__ volatile("repe cmpsb" : "+D"(p1), "+S"(p2), "+c"(count) : : "memory");

  if (count == 0)
    return 0;                        // all bytes matched
  return (p1[-1] < p2[-1]) ? -1 : 1; // proper signed compare
}

// fast_memset is now just memset, you can remove it
