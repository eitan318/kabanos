#include "va_allocation.h"
#include "memory_management/pmm.h"
#include "memory_management/vmm.h"
#include "stdio.h"
#include "utils/math.h"

bool va_alloc_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                     uint32_t flags) {
  uint32_t start = align_down(virt_start, PAGE_SIZE);
  uint32_t end = align_up(virt_start + size, PAGE_SIZE);
  uint32_t pages = (end - start) / PAGE_SIZE;

  for (uint32_t i = 0; i < pages; i++) {
    uint32_t phys = pmm_frame_alloc();
    if (!phys) {
      for (uint32_t j = 0; j < i; j++)
        vm_unmap(pd, start + j * PAGE_SIZE);
      return false;
    }

    if (!vm_map(pd, start + i * PAGE_SIZE, phys, flags)) {
      pmm_frame_free(phys);
      for (uint32_t j = 0; j < i; j++)
        vm_unmap(pd, start + j * PAGE_SIZE);
      return false;
    }
  }
  return true;
}

void va_free_region(page_dir_t *pd, uint32_t virt_start, size_t size) {
  uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  // Free physical frames before unmapping
  for (uint32_t i = 0; i < pages; i++) {
    uint32_t va = virt_start + i * PAGE_SIZE;
    uint32_t phys = virt_to_phys(pd, va);
    if (phys) {
      pmm_frame_free(phys);
    }
  }

  vm_unmap_range(pd, virt_start, size);
}
