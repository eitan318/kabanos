/**
 * @file va_allocation.c
 * @brief Backing virtual regions with physical frames (and undoing it).
 *
 * On partial failure va_alloc_region rolls back everything it mapped, so
 * the call is all-or-nothing.
 */
#include "mm/va_allocation.h"
#include "arch/types.h"
#include "hal.h"
#include "klib/stdio.h"
#include "mm/pmm.h"
#include "utils/math.h"
#include <assert.h>

bool va_alloc_region(arch_vm_t *vm, uint32_t virt_start, size_t size,
                     uint32_t flags) {
  ASSERT(vm);
  ASSERT(is_aligned(virt_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  uint32_t start = align_down(virt_start, PAGE_SIZE);
  uint32_t end = align_up(virt_start + size, PAGE_SIZE);
  uint32_t curr;

  for (curr = start; curr < end; curr += PAGE_SIZE) {
    uint32_t phys = pmm_frame_alloc();
    if (!phys)
      goto fail;

    if (!hal_vm_map(vm, curr, phys, flags)) {
      pmm_frame_free(phys);
      goto fail;
    }
  }
  return true;

fail:
  for (uint32_t clean = start; clean < curr; clean += PAGE_SIZE) {
    uint32_t p = hal_vm_virt_to_phys(vm, clean);
    if (p)
      pmm_frame_free(p);
    hal_vm_unmap(vm, clean);
  }
  return false;
}

void va_free_region(arch_vm_t *vm, uint32_t virt_start, size_t size) {
  ASSERT(vm);
  ASSERT(is_aligned(virt_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  size = align_up(size, PAGE_SIZE);
  uint32_t pages = size / PAGE_SIZE;

  for (uint32_t i = 0; i < pages; i++) {
    uint32_t va = virt_start + i * PAGE_SIZE;
    uint32_t phys = hal_vm_virt_to_phys(vm, va);
    if (phys) {
      pmm_frame_free(phys);
    }
  }

  hal_vm_unmap_range(vm, virt_start, size);
}
