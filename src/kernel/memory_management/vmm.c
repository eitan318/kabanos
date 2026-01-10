#include "memory_management/vmm.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "utils/math.h"
#include "vmm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KERNEL_PD_START (KERNEL_BASE / (4 * 1024 * 1024))

#define PD_PT_PRESENT PAGE_PRESENT
#define PD_PT_READWRITE PAGE_READWRITE
#define PD_PT_USER PAGE_USER

// Used for identity map low memory
static inline uint32_t *physical_access(uintptr_t paddr) {
  return (uint32_t *)(paddr);
}

static void tlb_flush(vaddr_t virtual_addr) {
  __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

paddr_t vm_translate(page_dir_t *pd, vaddr_t va) {
  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd[pd_index] & PD_PT_PRESENT)) {
    return 0; // not mapped
  }

  uint32_t *pt_virt = (uint32_t *)physical_access(pd[pd_index] & ~0xFFF);
  if (!(pt_virt[pt_index] & PD_PT_PRESENT)) {
    return 0;
  }

  return (pt_virt[pt_index] & ~0xFFF) + (va & 0xFFF);
}

bool vm_map(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags) {
  va = align_down(va, PAGE_SIZE);
  pa = align_down(pa, PAGE_SIZE);

  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
    paddr_t pt_phys = pmm_frame_alloc();

    if (!pt_phys) {

      debugf("Alloc FAILED The allocator was out of memory!");
      return false;
    }

    uint32_t *pt = physical_access(pt_phys);
    memset(pt, 0, PAGE_SIZE);

    pd_virt[pd_index] =
        pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
  }
  uint32_t *pt = physical_access(pd_virt[pd_index] & ~0xFFF);
  pt[pt_index] = pa | (flags & 0xFFF) | PD_PT_PRESENT;

  __asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
  return true;
}

bool vm_unmap(page_dir_t *pd_virt, vaddr_t virt_addr) {
  virt_addr = align_down(virt_addr, PAGE_SIZE);

  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = (virt_addr >> 12) & 0x3FF;

  if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
    return false;
  }
  uint32_t *pt_virt = physical_access(pd_virt[pd_index] & ~0xFFF);
  if (!(pt_virt[pt_index] & PD_PT_PRESENT)) {
    return false;
  }

  paddr_t pa = pt_virt[pt_index] & ~0xFFF;
  pt_virt[pt_index] = 0;

  tlb_flush(virt_addr);
  pmm_frame_free(pa);
  return true;
}

void vmspace_switch(vmspace_t *vmspace) {
  asm volatile("mov %0, %%cr3" ::"r"(vmspace->pd_phys));
}

void pd_destroy(page_dir_t *pd) {
  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (pd[i] & PD_PT_PRESENT) {
      paddr_t pt_phys = pd[i] & ~0xFFF;
      pmm_frame_free(pt_phys);
    }
  }
}
