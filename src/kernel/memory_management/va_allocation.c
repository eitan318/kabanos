#include "va_allocation.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"

static uint32_t va_start = 0;
static uint32_t va_end = 0;
static uint32_t va_next = 0;
static PageDirectory *va_page_dir = NULL;

void va_allocator_init(uint32_t start, uint32_t end, PageDirectory *page_dir) {
  va_start = start;
  va_end = end;
  va_next = start;
  va_page_dir = page_dir;
}

// Allocate `size` bytes, rounded up to page boundary, mapped to physical frames
void *va_alloc(size_t size, bool to_zero) {
  if (!va_page_dir || size == 0)
    return NULL;

  size_t pages_needed = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t alloc_start = va_next;

  if (alloc_start + pages_needed * PAGE_SIZE > va_end) {
    return NULL; // out of reserved VA
  }

  uint32_t virt = alloc_start;
  for (size_t i = 0; i < pages_needed; i++) {
    uint64_t phys = frame_alloc();
    if (!phys) {
      // Rollback
      for (size_t j = 0; j < i; j++) {
        uint32_t p = alloc_start + j * PAGE_SIZE;
        paging_unmap(va_page_dir, p);
      }
      return NULL;
    }

    if (!paging_map(va_page_dir, virt, (uint32_t)phys, PAGE_WRITABLE)) {
      // Rollback
      frame_free(phys);
      for (size_t j = 0; j < i; j++) {
        uint32_t p = alloc_start + j * PAGE_SIZE;
        uint32_t phys_prev = paging_get_physical(va_page_dir, p);
        paging_unmap(va_page_dir, p);
        frame_free(phys_prev);
      }
      return NULL;
    }

    if (to_zero) {
      uint8_t *ptr = (uint8_t *)virt;
      for (size_t k = 0; k < PAGE_SIZE; k++)
        ptr[k] = 0;
    }

    virt += PAGE_SIZE;
  }

  va_next += pages_needed * PAGE_SIZE;
  return (void *)alloc_start;
}

// Optional: simple stub, real implementation needs free list
void va_free(void *ptr, size_t size) {
  (void)ptr;
  (void)size;
  // Could implement later with a free list
}
