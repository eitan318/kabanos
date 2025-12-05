#include "memory.h"

void *memcpy(void *dst, const void *src, uint32_t num) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;
  for (uint32_t i = 0; i < num; i++)
    d[i] = s[i];
  return dst;
}

void *memset(void *ptr, int value, uint32_t num) {
  uint8_t *p = (uint8_t *)ptr;
  for (uint32_t i = 0; i < num; i++)
    p[i] = (uint8_t)value;
  return ptr;
}

int memcmp(const void *ptr1, const void *ptr2, uint32_t num) {
  const uint8_t *p1 = (const uint8_t *)ptr1;
  const uint8_t *p2 = (const uint8_t *)ptr2;
  for (uint32_t i = 0; i < num; i++)
    if (p1[i] != p2[i])
      return 1;
  return 0;
}
