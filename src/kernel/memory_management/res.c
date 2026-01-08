#include "boot_allocator.h"
#include "include/memory.h"
#include "kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "utils/math.h"
#include "vmm.h"

#define KERNEL_PD_START (KERNEL_BASE / (4 * 1024 * 1024))
#define BOOT_ID_MAP_SIZE (16 * 1024 * 1024) // 16MB identity map

#define PD_PT_PRESENT PAGE_PRESENT
#define PD_PT_READWRITE PAGE_READWRITE
#define PD_PT_USER PAGE_USER

// STEP 1: Helper to access physical pages through identity mapping
// While paging is on, low physical memory is identity mapped
static inline uint32_t *phys_access(uintptr_t paddr) {
  // During boot: paging OFF → use physical address directly
  // After boot: paging ON → use identity mapping (0-16MB mapped to 0-16MB)
  return (uint32_t *)paddr;
}

// STEP 2: Map a single page (used before AND after paging enabled)
static bool vm_map_early(uint32_t *pd, vaddr_t va, paddr_t pa, uint32_t flags) {
  va = align_down(va, PAGE_SIZE);
  pa = align_down(pa, PAGE_SIZE);

  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  // Check if page table exists
  if (!(pd[pd_index] & PD_PT_PRESENT)) {
    // Allocate page table from LOW memory (< BOOT_ID_MAP_SIZE)
    paddr_t pt_phys = (paddr_t)boot_alloc(PAGE_SIZE);
    if (pt_phys >= BOOT_ID_MAP_SIZE) {
      return false; // MUST be in identity-mapped region
    }

    uint32_t *pt = phys_access(pt_phys);
    memset(pt, 0, PAGE_SIZE);
    pd[pd_index] =
        pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
  }

  uint32_t *pt = phys_access(pd[pd_index] & ~0xFFF);
  pt[pt_index] = pa | (flags & 0xFFF) | PD_PT_PRESENT;

  return true;
}

// STEP 3: Identity map a range
static void map_range_identity(uint32_t *pd, paddr_t start, size_t size) {
  for (paddr_t pa = align_down(start, PAGE_SIZE);
       pa < align_up(start + size, PAGE_SIZE); pa += PAGE_SIZE) {
    vm_map_early(pd, pa, pa, PD_PT_PRESENT | PD_PT_READWRITE);
  }
}

// STEP 4: Map physical range to virtual range
static void map_range(uint32_t *pd, vaddr_t va, paddr_t pa, size_t size,
                      uint32_t flags) {
  for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
    vm_map_early(pd, va + offset, pa + offset, flags);
  }
}

// STEP 5: Create initial page directory (PAGING OFF)
vmspace_t *kernel_vmspace_creat() {
  vmspace_t *vmspace = boot_alloc(sizeof(*vmspace));
  if (!vmspace)
    return NULL;

  // Allocate page directory from LOW memory
  paddr_t pd_phys = (paddr_t)boot_alloc(PAGE_SIZE);
  if (pd_phys >= BOOT_ID_MAP_SIZE)
    return NULL;

  vmspace->pd = (uint32_t *)pd_phys; // Valid when paging OFF
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  // STEP 6: Identity map low memory (0-16MB)
  // This allows us to access page structures after paging is enabled
  map_range_identity(vmspace->pd, 0, BOOT_ID_MAP_SIZE);

  // STEP 7: Map kernel to higher half
  extern uint8_t _kernel_start, _kernel_end;
  paddr_t kernel_phys_start = (uintptr_t)&_kernel_start - KERNEL_BASE;
  size_t kernel_size = (uintptr_t)&_kernel_end - (uintptr_t)&_kernel_start;

  map_range(vmspace->pd,
            (vaddr_t)&_kernel_start, // Virtual: 0xC0100000+
            kernel_phys_start,       // Physical: 0x100000+
            kernel_size, PD_PT_PRESENT | PD_PT_READWRITE);

  // NO RECURSIVE MAPPING - we use identity mapping instead

  return vmspace;
}

// STEP 8: After paging enabled, use identity mapping to access structures
paddr_t vm_translate(page_dir_t *pd, vaddr_t va) {
  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd[pd_index] & PD_PT_PRESENT))
    return 0;

  uint32_t *pt = phys_access(pd[pd_index] & ~0xFFF);
  if (!(pt[pt_index] & PD_PT_PRESENT))
    return 0;

  return (pt[pt_index] & ~0xFFF) + (va & 0xFFF);
}

// STEP 9: Map pages after paging is enabled
bool vm_map(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags) {
  va = align_down(va, PAGE_SIZE);
  pa = align_down(pa, PAGE_SIZE);

  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
    // Allocate from PMM (must return frames < BOOT_ID_MAP_SIZE for now)
    paddr_t pt_phys = pmm_frame_alloc();
    if (pt_phys >= BOOT_ID_MAP_SIZE) {
      // TODO: use temporary mapping or expand identity map
      return false;
    }

    uint32_t *pt = phys_access(pt_phys);
    memset(pt, 0, PAGE_SIZE);
    pd_virt[pd_index] =
        pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
  }

  uint32_t *pt = phys_access(pd_virt[pd_index] & ~0xFFF);
  pt[pt_index] = pa | (flags & 0xFFF) | PD_PT_PRESENT;

  __asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
  return true;
}

bool vm_unmap(page_dir_t *pd_virt, vaddr_t va) {
  va = align_down(va, PAGE_SIZE);

  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd_virt[pd_index] & PD_PT_PRESENT))
    return false;

  uint32_t *pt = phys_access(pd_virt[pd_index] & ~0xFFF);
  if (!(pt[pt_index] & PD_PT_PRESENT))
    return false;

  paddr_t pa = pt[pt_index] & ~0xFFF;
  pt[pt_index] = 0;

  __asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
  pmm_frame_free(pa);
  return true;
}

// STEP 10: Create user process page directory
vmspace_t *user_vmspace_creat() {
  vmspace_t *vmspace = kmalloc(sizeof(*vmspace));
  if (!vmspace)
    return NULL;

  // Allocate PD from low memory
  paddr_t pd_phys = pmm_frame_alloc();
  if (pd_phys >= BOOT_ID_MAP_SIZE) {
    kfree(vmspace);
    return NULL;
  }

  vmspace->pd = phys_access(pd_phys);
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
  if (vmspace->pd == g_kernel_vmspace->pd)
    return;

  // Free only USER page tables (not kernel ones)
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
  __asm__ volatile("mov %0, %%cr3" : : "r"(vmspace->pd_phys));
}
