#include "va_allocation.h"
#include "arch/types.h"
#include "hal.h"
#include "memory_management/pmm.h"
#include "stdio.h"
#include "utils/math.h"

bool va_alloc_region(arch_vm_t *vm, uint32_t virt_start, size_t size,
                     uint32_t flags) {
  uint32_t start = align_down(virt_start, PAGE_SIZE);
  uint32_t end = align_up(virt_start + size, PAGE_SIZE);
  uint32_t pages = (end - start) / PAGE_SIZE;

  for (uint32_t i = 0; i < pages; i++) {
    uint32_t phys = pmm_frame_alloc();
    if (!phys) {
      for (uint32_t j = 0; j < i; j++)
        hal_vm_unmap(vm, start + j * PAGE_SIZE);
      return false;
    }

    if (!hal_vm_map(vm, start + i * PAGE_SIZE, phys, flags)) {
      pmm_frame_free(phys);
      for (uint32_t j = 0; j < i; j++)
        hal_vm_unmap(vm, start + j * PAGE_SIZE);
      return false;
    }
  }
  return true;
}

void va_free_region(arch_vm_t *vm, uint32_t virt_start, size_t size) {
  uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  // Free physical frames before unmapping
  for (uint32_t i = 0; i < pages; i++) {
    uint32_t va = virt_start + i * PAGE_SIZE;
    uint32_t phys = hal_vm_virt_to_phys(vm, va);
    if (phys) {
      pmm_frame_free(phys);
    }
  }

  hal_vm_unmap_range(vm, virt_start, size);
}
