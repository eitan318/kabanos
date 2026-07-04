/**
 * @file malloc.h
 * @brief Bump allocator over the fixed stage2 heap region.
 *
 * free() is a no-op; memory is reclaimed only by rebooting. Definitions
 * live in this header, so it must be included by exactly one translation
 * unit.
 */
#include "memdefs.h"
#include "stddef.h"
#include "stdint.h"

uint8_t *heap_ptr = (uint8_t *)MEMORY_HEAP_START;

void *malloc(size_t size) {
  size = (size + 7) & ~7;

  void *res = heap_ptr;
  heap_ptr += size;

  // TODO: fail once the heap region (MEMORY_HEAP_SIZE) is exhausted
  return res;
}

void free(void *ptr) {}
