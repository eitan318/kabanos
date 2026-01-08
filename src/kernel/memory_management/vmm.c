#include "vmm.h"
#include "boot_allocator.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "utils/math.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ucontext.h>

#define KERNEL_PD_START (KERNEL_BASE / (4 * 1024 * 1024))

#define PD_PT_PRESENT PAGE_PRESENT
#define PD_PT_READWRITE PAGE_READWRITE
#define PD_PT_USER PAGE_USER

#define BOOT_ID_MAP_SIZE (16 * 1024 * 1024) // 16MB

// Used for identity map low memory
static inline uint32_t *physical_access(uintptr_t paddr) {
  return (uint32_t *)(paddr);
}

// assuming pd mapped into kernel space
paddr_t vm_translate(page_dir_t *pd, vaddr_t va) {
  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  uint32_t *pd_virt = (uint32_t *)pd;
  if (!(pd_virt[pd_index] & PD_PT_PRESENT))
    return 0; // not mapped

  uint32_t *pt_virt = (uint32_t *)physical_access(pd_virt[pd_index] & ~0xFFF);
  if (!(pt_virt[pt_index] & PD_PT_PRESENT))
    return 0;

  return (pt_virt[pt_index] & ~0xFFF) + (va & 0xFFF);
}

// create vmspace for user processes
vmspace_t *user_vmspace_creat() {
  vmspace_t *vmspace = kmalloc(sizeof(*vmspace));
  if (!vmspace)
    return NULL;

  paddr_t pd_phys = pmm_frame_alloc();
  if (!pd_phys) {
    kfree(vmspace);
    return NULL;
  }

  vmspace->pd = physical_access(pd_phys);
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  extern vmspace_t *g_kernel_vmspace;
  memcpy(&vmspace->pd[KERNEL_PD_START], &g_kernel_vmspace->pd[KERNEL_PD_START],
         (PD_ENTRIES - KERNEL_PD_START) * sizeof(uint32_t));

  return vmspace;
}

// create initial page dir for kernel and recursively maps it
vmspace_t *kernel_vmspace_creat() {
  vmspace_t *vmspace = boot_alloc(sizeof(*vmspace));
  if (!vmspace)
    return NULL;

  paddr_t pd_phys = (paddr_t)boot_alloc(PAGE_SIZE);

  vmspace->pd = physical_access(pd_phys);
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  // map kernel code/data/bss
  extern uint8_t _kernel_start, _kernel_end;
  for (uintptr_t pa = align_down((uintptr_t)&_kernel_start, PAGE_SIZE);
       pa < align_up((uintptr_t)&_kernel_end, PAGE_SIZE); pa += PAGE_SIZE) {
    vm_map(vmspace->pd, KERNEL_BASE + (pa - (uintptr_t)&_kernel_start), pa,
           PD_PT_PRESENT | PD_PT_READWRITE);
  }

  return vmspace;
}

// should be used only on user vm space, kernel vmspace should
void vmspace_destroy(vmspace_t *vmspace) {
  if (!vmspace) {
    return;
  }

  extern vmspace_t *g_kernel_vmspace;
  if (vmspace->pd == g_kernel_vmspace->pd) {
    // kernel vmspace will never be freed because they were early
    // kernel allocated.
    return;
  }

  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (vmspace->pd[i] & PD_PT_PRESENT) {
      paddr_t pt_phys = vmspace->pd[i] & ~0xFFF;
      pmm_frame_free(pt_phys);
    }
  }

  pmm_frame_free(vmspace->pd_phys);
  kfree(vmspace);
}

static void tlb_flush(vaddr_t virtual_addr) {
  __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

bool vm_map(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags) {
  va = align_down(va, PAGE_SIZE);
  pa = align_down(pa, PAGE_SIZE);

  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  debugf("PDE[%u] = %x\n", pd_index, pd_virt[pd_index]);
  if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
    paddr_t pt_phys = pmm_frame_alloc();
    uint32_t *pt_virt = physical_access(pt_phys);
    memset(pt_virt, 0, PAGE_SIZE);
    uint32_t new_pde =
        pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
    pd_virt[pd_index] = new_pde;
  }

  uint32_t *pt_virt = physical_access(pd_virt[pd_index] & ~0xFFF);
  pt_virt[pt_index] = pa | (flags & 0xFFF) | PD_PT_PRESENT;

  tlb_flush(va);
  return true;
}
//
// bool vm_map_early(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t
// flags) {
//   va = align_down(va, PAGE_SIZE);
//   pa = align_down(pa, PAGE_SIZE);
//   uint32_t pd_index = va >> 22;
//   uint32_t pt_index = (va >> 12) & 0x3FF;
//   if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
//     paddr_t pt_phys = (paddr_t)boot_alloc(PAGE_SIZE);
//     if (pt_phys > BOOT_ID_MAP_SIZE) {
//       return false;
//     }
//     uint32_t *pt_virt = physical_access(pt_phys);
//     memset(pt_virt, 0, PAGE_SIZE);
//     uint32_t new_pde =
//         pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
//     pd_virt[pd_index] = new_pde;
//   }
//   uint32_t *pt_virt = physical_access(pd_virt[pd_index] & ~0xFFF);
//   pt_virt[pt_index] = pa | (flags & 0xFFF) | PD_PT_PRESENT;
//   tlb_flush(va);
//   return true;
// }
//
// bool vm_map_helper(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t
// flags, bool early) {
//   va = align_down(va, PAGE_SIZE);
//   pa = align_down(pa, PAGE_SIZE);
//
//   uint32_t pd_index = va >> 22;
//   uint32_t pt_index = (va >> 12) & 0x3FF;
//
//   if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
//     paddr_t pt_phys = early ? boot_alloc(PAGE_SIZE) : pmm_frame_alloc();
//     if (pt_phys >= BOOT_ID_MAP_SIZE) {
//       debugf("[ERROR] addr passed identity map range");
//       return false;
//     }
//
//     uint32_t *pt = physical_access(pt_phys);
//     memset(pt, 0, PAGE_SIZE);
//     pd_virt[pd_index] = pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags &
//     PD_PT_USER);
//   }
//
//   uint32_t *pt = physical_access(pd_virt[pd_index] & ~0xFFF);
//   pt[pt_index] = pa | (flags & 0xFFF) | PD_PT_PRESENT;
//
//   __asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
//   return true;
// }
bool vm_unmap(page_dir_t *pd_virt, vaddr_t virtual_addr) {
  virtual_addr = (vaddr_t)align_down((uintptr_t)virtual_addr, PAGE_SIZE);

  uint32_t pd_index = virtual_addr >> 22;
  uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;

  if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
    return false;
  }

  uint32_t *pt_virt = physical_access(pd_virt[pd_index] & ~0xFFF);
  if (!(pt_virt[pt_index] & PD_PT_PRESENT)) {
    return false;
  }
  paddr_t phys = pt_virt[pt_index] & ~0xFFF;
  pt_virt[pt_index] = 0;
  pmm_frame_free(phys);
  tlb_flush(virtual_addr);
  return true;
}

void vmspace_switch(vmspace_t *vmspace) {
  uint32_t cr3_phys = vmspace->pd_phys;
  asm volatile("mov %0, %%cr3" ::"r"(cr3_phys));
}
