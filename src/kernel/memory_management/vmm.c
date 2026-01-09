#include "memory_management/vmm.h"
#include "early_pmm.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "kmalloc.h"
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

bool vm_map_helper(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags,
                   bool early) {
  va = align_down(va, PAGE_SIZE);
  pa = align_down(pa, PAGE_SIZE);

  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
    paddr_t pt_phys =
        early ? (paddr_t)early_pmm_alloc(PAGE_SIZE) : pmm_frame_alloc();

    if (!pt_phys || pt_phys >= EARLY_IDENTITY_MAP_SIZE) {

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

bool vm_map_early(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags) {
  return vm_map_helper(pd_virt, va, pa, flags, true);
}

// Used to identity-map a range of physical addrs in early kernel
static void map_range_identity(page_dir_t *pd_virt, paddr_t range_start,
                               size_t range_size, uint32_t flags,
                               bool is_early) {

  for (size_t offset = 0; offset < range_size; offset += PAGE_SIZE) {
    vm_map_helper(pd_virt, range_start + offset, range_start + offset, flags,
                  is_early);
  }
}

// Map physical range to virtual range
static void map_range(page_dir_t *pd_virt, paddr_t pa_start, vaddr_t va_start,
                      size_t size, uint32_t flags, bool is_early) {
  for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
    vm_map_helper(pd_virt, va_start + offset, pa_start + offset, flags,
                  is_early);
  }
}

bool vm_map(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags) {
  return vm_map_helper(pd_virt, va, pa, flags, false);
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

// Create initial virtual memory space for kernel
void kernel_vmspace_creat(vmspace_t *vmspace) {
  paddr_t pd_phys = (paddr_t)pmm_frame_alloc();
  if (!pd_phys)
    return;

  vmspace->pd = physical_access(pd_phys);
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  // map kernel code/data/bss
  map_range_identity(vmspace->pd, 0, EARLY_IDENTITY_MAP_SIZE, PAGE_READWRITE,
                     false);

  // Map kernel code to heigher helf
  extern uint8_t _kernel_start, _kernel_end;
  paddr_t kernel_phys_start = (uintptr_t)&_kernel_start - KERNEL_BASE;

  paddr_t kernel_size = (uintptr_t)&_kernel_end - (uintptr_t)&_kernel_start;
  map_range(vmspace->pd, kernel_phys_start, (vaddr_t)&_kernel_start,
            kernel_size, PD_PT_READWRITE, false);
}

// Create virtual memory space for user processes
vmspace_t *user_vmspace_creat() {
  vmspace_t *vmspace = kmalloc(sizeof(*vmspace));
  if (!vmspace) {
    return NULL;
  }
  // TODO: check if the frame allocator allocating from all avail memory range
  // is problematic
  paddr_t pd_phys = pmm_frame_alloc();
  if (!pd_phys || pd_phys >= EARLY_IDENTITY_MAP_SIZE) {
    kfree(vmspace);
    return NULL;
  }

  vmspace->pd = physical_access(pd_phys);
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  // Copy kernel mappings (including identity map)
  extern vmspace_t *g_kernel_vmspace;
  memcpy(vmspace->pd, g_kernel_vmspace->pd, PAGE_SIZE);

  return vmspace;
}

void vmspace_destroy(vmspace_t *vmspace) {
  if (!vmspace)
    return;
  extern vmspace_t *g_kernel_vmspace;

  // kernel vmspace shall not be freed because it is early-kernel-allocated
  if (vmspace->pd == g_kernel_vmspace->pd)
    return;

  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (vmspace->pd[i] & PD_PT_PRESENT) {
      paddr_t pt_phys = vmspace->pd[i] & ~0xFFF;
      pmm_frame_free(pt_phys);
    }
  }

  pmm_frame_free(vmspace->pd_phys);
  kfree(vmspace);
}

void vmspace_switch(vmspace_t *vmspace) {
  asm volatile("mov %0, %%cr3" ::"r"(vmspace->pd_phys));
}
