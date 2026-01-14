#include "va_allocation.h"
#include "include/stdio.h"
#include "memory_management/pmm.h"
#include "memory_management/vmm.h"

void va_free_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                    bool map_down) {
  uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  // Free physical frames before unmapping
  for (uint32_t i = 0; i < pages; i++) {
    uint32_t va;
    if (map_down) {
      va = virt_start - (i + 1) * PAGE_SIZE;
    } else {
      va = virt_start + i * PAGE_SIZE;
    }
    uint32_t phys = virt_to_phys(pd, va);
    if (phys) {
      pmm_frame_free(phys);
    }
  }

  // Unmap the entire range efficiently
  uint32_t unmap_start =
      map_down ? (virt_start - pages * PAGE_SIZE) : virt_start;
  vm_unmap_range(pd, unmap_start, size);
}

bool va_alloc_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                     uint32_t flags, bool map_down) {
  uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  // Allocate contiguous physical frames
  uint32_t phys_start = pmm_frame_alloc();
  if (!phys_start)
    return false;

  uint32_t mapped = 1;
  for (uint32_t i = 1; i < pages; i++) {
    uint32_t phys = pmm_frame_alloc();
    if (!phys)
      goto rollback;
    mapped++;
  }

  // Map the entire range efficiently
  uint32_t map_start = map_down ? (virt_start - pages * PAGE_SIZE) : virt_start;
  if (!vm_map_range(pd, phys_start, map_start, size, flags)) {
    mapped = pages;
    goto rollback;
  }

  return true;

rollback:
  // Free all allocated physical frames
  for (uint32_t i = 0; i < mapped; i++) {
    pmm_frame_free(phys_start + i * PAGE_SIZE);
  }
  return false;
}
