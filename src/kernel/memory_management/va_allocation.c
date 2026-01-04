#include "va_allocation.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"

bool va_alloc_region(PageDirectory *pd, uint32_t virt_start, size_t size,
                     uint32_t flags, bool map_down) {
  uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t mapped = 0;

  for (uint32_t i = 0; i < pages; i++) {
    uint32_t va;

    if (map_down) {
      va = virt_start - (i + 1) * PAGE_SIZE;
    } else {
      va = virt_start + i * PAGE_SIZE;
    }

    uint32_t phys = frame_alloc();
    if (!phys)
      goto rollback;

    if (!paging_map(pd, va, phys, flags)) {
      frame_free(phys);
      goto rollback;
    }

    mapped++;
  }

  return true;

rollback:
  for (uint32_t i = 0; i < mapped; i++) {
    uint32_t va;

    if (map_down) {
      va = virt_start - (i + 1) * PAGE_SIZE;
    } else {
      va = virt_start + i * PAGE_SIZE;
    }

    uint32_t phys = paging_get_physical(pd, va);
    if (phys) {
      paging_unmap(pd, va);
      frame_free(phys);
    }
  }

  return false;
}
