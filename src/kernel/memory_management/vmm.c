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

static void tlb_flush(vaddr_t virtual_addr) {
  __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

paddr_t vm_translate(page_dir_t *pd, vaddr_t va) {
  uint32_t pd_index = va >> 22;
  uint32_t pt_index = (va >> 12) & 0x3FF;

  if (!(pd[pd_index] & PD_PT_PRESENT)) {
    return 0; // not mapped
  }

  paddr_t pt_phys = (pd[pd_index] & ~0xFFF);
  uint32_t *pt_virt = (uint32_t *)(pt_phys + KERNEL_BASE);
  if (!(pt_virt[pt_index] & PD_PT_PRESENT)) {
    return 0;
  }

  return (pt_virt[pt_index] & ~0xFFF) + (va & 0xFFF);
}

// Efficient range mapping
bool vm_map_range(page_dir_t *pd_virt, paddr_t pa_start, vaddr_t va_start,
                  size_t size, uint32_t flags) {
  if (size == 0)
    return true;

  va_start = align_down(va_start, PAGE_SIZE);
  pa_start = align_down(pa_start, PAGE_SIZE);
  size = align_up(size, PAGE_SIZE);

  vaddr_t va = va_start;
  paddr_t pa = pa_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = va >> 22;
    uint32_t pt_index = (va >> 12) & 0x3FF;

    // Calculate how much we can map in this page table
    uint32_t remaining_in_pt = (1024 - pt_index) * PAGE_SIZE;
    uint32_t remaining_total = va_end - va;
    uint32_t map_size =
        (remaining_in_pt < remaining_total) ? remaining_in_pt : remaining_total;

    // Check if we can map an entire page table at once
    if (pt_index == 0 && map_size >= (1024 * PAGE_SIZE)) {
      uint32_t *pt_virt = NULL;
      paddr_t pt_phys;
      if (pd_virt[pd_index] & PD_PT_PRESENT) {
        // Reuse existing PT instead of allocating
        pt_phys = pd_virt[pd_index] & ~0xFFF;
        pt_virt = (uint32_t *)(pt_phys + KERNEL_BASE); // Remove "uint32_t *"
      } else {
        // Fast path: map entire 4MB region
        pt_phys = pmm_frame_alloc();
        if (!pt_phys) {
          debugf("Alloc FAILED The allocator was out of memory!");
          return false;
        }
        pt_virt = (uint32_t *)(pt_phys + KERNEL_BASE);
      }
      // Fill entire page table
      for (uint32_t i = 0; i < 1024; i++) {
        pt_virt[i] = (pa + i * PAGE_SIZE) | (flags & 0xFFF) | PD_PT_PRESENT;
      }

      pd_virt[pd_index] =
          pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);

      va += 1024 * PAGE_SIZE;
      pa += 1024 * PAGE_SIZE;
    } else {
      // Slow path: map partial page table
      // Ensure page table exists
      if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
        paddr_t pt_phys = pmm_frame_alloc();
        if (!pt_phys) {
          debugf("Alloc FAILED The allocator was out of memory!");
          return false;
        }
        uint32_t *pt_virt = (uint32_t *)(pt_phys + KERNEL_BASE);
        memset(pt_virt, 0, PAGE_SIZE);
        pd_virt[pd_index] =
            pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
      }

      uint32_t pt_phys = pd_virt[pd_index] & ~0xFFF;
      uint32_t *pt_virt = (uint32_t *)(pt_phys + KERNEL_BASE);
      uint32_t pages = map_size / PAGE_SIZE;

      // Map pages in this page table
      for (uint32_t i = 0; i < pages; i++) {
        pt_virt[pt_index + i] =
            (pa + i * PAGE_SIZE) | (flags & 0xFFF) | PD_PT_PRESENT;
        tlb_flush(va + i * PAGE_SIZE);
      }

      va += map_size;
      pa += map_size;
    }
  }

  if (size > PAGE_SIZE) {
    // Flush TLB for entire range (CR3 reload for efficiency)
    __asm__ volatile("mov %%cr3, %%eax\n"
                     "mov %%eax, %%cr3\n" ::
                         : "eax", "memory");
  }

  return true;
}

// Efficient range unmapping
bool vm_unmap_range(page_dir_t *pd_virt, vaddr_t va_start, size_t size) {
  if (size == 0)
    return true;

  va_start = align_down(va_start, PAGE_SIZE);
  size = align_up(size, PAGE_SIZE);

  vaddr_t va = va_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = va >> 22;
    uint32_t pt_index = (va >> 12) & 0x3FF;

    if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
      // Skip to next page table boundary
      va = (pd_index + 1) << 22;
      continue;
    }

    // Calculate how much we can unmap in this page table
    uint32_t remaining_in_pt = (1024 - pt_index) * PAGE_SIZE;
    uint32_t remaining_total = va_end - va;
    uint32_t unmap_size =
        (remaining_in_pt < remaining_total) ? remaining_in_pt : remaining_total;

    uint32_t pt_phys = pd_virt[pd_index] & ~0xFFF;
    uint32_t *pt_virt = (uint32_t *)(pt_phys + KERNEL_BASE);

    // Check if we're unmapping an entire page table
    if (pt_index == 0 && unmap_size >= (1024 * PAGE_SIZE)) {
      pmm_frame_free(pt_phys);
      // Clear page directory entry
      pd_virt[pd_index] = 0;

      va += 1024 * PAGE_SIZE;
    } else {
      // Slow path: unmap partial page table
      uint32_t pages = unmap_size / PAGE_SIZE;

      for (uint32_t i = 0; i < pages; i++) {
        if (pt_virt[pt_index + i] & PD_PT_PRESENT) {
          pt_virt[pt_index + i] = 0;
        }
      }

      va += unmap_size;
    }
  }

  if (size > PAGE_SIZE) {
    // Flush TLB for entire range (CR3 reload for efficiency)
    __asm__ volatile("mov %%cr3, %%eax\n"
                     "mov %%eax, %%cr3\n" ::
                         : "eax", "memory");
  }
  return true;
}

bool vm_map(page_dir_t *pd_virt, vaddr_t va, paddr_t pa, uint32_t flags) {
  bool res = vm_map_range(pd_virt, pa, va, PAGE_SIZE, flags);
  if (res) {
    tlb_flush(va);
  }
  return res;
}

bool vm_unmap(page_dir_t *pd_virt, vaddr_t virt_addr) {
  bool res = vm_unmap_range(pd_virt, virt_addr, PAGE_SIZE);
  if (res) {
    tlb_flush(virt_addr);
  }
  return res;
}

void pd_destroy(page_dir_t *pd) {
  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (pd[i] & PD_PT_PRESENT) {
      paddr_t pt_phys = pd[i] & ~0xFFF;
      pmm_frame_free(pt_phys);
    }
  }
}
