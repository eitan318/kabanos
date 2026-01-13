#include "va_allocation.h"
#include "include/stdio.h"
#include "memory_management/pmm.h"
#include "memory_management/vmm.h"

void va_free_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                    bool map_down) {
  uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  for (uint32_t i = 0; i < pages; i++) {
    uint32_t va;

    if (map_down) {
      va = virt_start - (i + 1) * PAGE_SIZE;
    } else {
      va = virt_start + i * PAGE_SIZE;
    }

    uint32_t phys = vm_translate(pd, va);
    if (phys) {
      vm_unmap(pd, va);
      pmm_frame_free(phys);
    }
  }
}

bool va_alloc_region(page_dir_t *pd, uint32_t virt_start, size_t size,
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

    uint32_t phys = pmm_frame_alloc();
    if (!phys)
      goto rollback;

    if (!vm_map(pd, va, phys, flags)) {
      pmm_frame_free(phys);
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

    uint32_t phys = vm_translate(pd, va);
    if (phys) {
      vm_unmap(pd, va);
      pmm_frame_free(phys);
    }
  }

  return false;
}
