#include "memdefs.h"
#include "stddef.h"
#include "stdint.h"

uint8_t *heap_ptr = (uint8_t *)MEMORY_HEAP_START;

void *malloc(size_t size) {
  size = (size + 7) & ~7;

  void *res = heap_ptr;
  heap_ptr += size;

  // check if hit start of end of ram
  return res;
}

void free(void *ptr) {}
