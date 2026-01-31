#include "arch/i686/types.h"
#include "arch/types.h"
#include "assert.h"
#include "hal.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "stdio.h"
#include "string.h"
#include "utils/math.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t page_dir_t;

#define KERNEL_PD_START (KERNEL_BASE / (4 * 1024 * 1024))
#define PD_PT_PRESENT PAGE_PRESENT
#define PD_PT_READWRITE PAGE_READWRITE
#define PD_PT_USER PAGE_USER
#define PT_ENTRIES 1024
#define PT_SIZE (PT_ENTRIES * PAGE_SIZE)

static void tlb_flush(vaddr_t virtual_addr) {
  __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

static void tlb_flush_all(void) {
  __asm__ volatile("mov %%cr3, %%eax\n"
                   "mov %%eax, %%cr3\n" ::
                       : "eax", "memory");
}

static inline uint32_t get_pd_index(vaddr_t va) { return va >> 22; }

static inline uint32_t get_pt_index(vaddr_t va) { return (va >> 12) & 0x3FF; }

static inline uint32_t *get_pt_virtual(paddr_t pt_phys) {
  return (uint32_t *)(pt_phys + KERNEL_BASE);
}

static inline uint32_t calc_remaining_in_pt(uint32_t pt_index) {
  return (PT_ENTRIES - pt_index) * PAGE_SIZE;
}

paddr_t hal_vm_virt_to_phys(arch_vm_t *vm, vaddr_t va) {
  page_dir_t *pd = vm->pd;
  uint32_t pd_index = get_pd_index(va);
  uint32_t pt_index = get_pt_index(va);

  if (!(pd[pd_index] & PD_PT_PRESENT)) {
    return 0;
  }

  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  uint32_t *pt_virt = get_pt_virtual(pt_phys);

  if (!(pt_virt[pt_index] & PD_PT_PRESENT)) {
    return 0;
  }

  return (pt_virt[pt_index] & ~0xFFF) + (va & 0xFFF);
}

//
// Page Table Allocation and Setup
//

static paddr_t allocate_page_table(void) {
  paddr_t pt_phys = pmm_frame_alloc();
  if (!pt_phys) {
    debugf("Alloc FAILED The allocator was out of memory!");
    return 0;
  }

  uint32_t *pt_virt = get_pt_virtual(pt_phys);
  memset(pt_virt, 0, PAGE_SIZE);
  return pt_phys;
}

static bool ensure_page_table_exists(page_dir_t *pd, uint32_t pd_index,
                                     uint32_t flags) {
  if (pd[pd_index] & PD_PT_PRESENT) {
    return true;
  }

  paddr_t pt_phys = allocate_page_table();
  if (!pt_phys) {
    return false;
  }

  pd[pd_index] =
      pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
  return true;
}

static paddr_t get_or_create_page_table(page_dir_t *pd, uint32_t pd_index,
                                        uint32_t flags, bool *created) {
  if (pd[pd_index] & PD_PT_PRESENT) {
    *created = false;
    return pd[pd_index] & ~0xFFF;
  }

  paddr_t pt_phys = allocate_page_table();
  if (!pt_phys) {
    return 0;
  }

  pd[pd_index] =
      pt_phys | PD_PT_PRESENT | PD_PT_READWRITE | (flags & PD_PT_USER);
  *created = true;
  return pt_phys;
}

//
// Page Table Mapping Operations
//

static void fill_page_table_entries(uint32_t *pt, uint32_t start_index,
                                    uint32_t count, paddr_t pa_base,
                                    uint32_t flags) {
  for (uint32_t i = 0; i < count; i++) {
    pt[start_index + i] =
        (pa_base + i * PAGE_SIZE) | (flags & 0xFFF) | PD_PT_PRESENT;
  }
}

static void map_full_page_table(page_dir_t *pd, uint32_t pd_index, paddr_t pa,
                                uint32_t flags) {
  bool created;
  paddr_t pt_phys = get_or_create_page_table(pd, pd_index, flags, &created);
  uint32_t *pt_virt = get_pt_virtual(pt_phys);

  fill_page_table_entries(pt_virt, 0, PT_ENTRIES, pa, flags);
}

static void map_partial_page_table(page_dir_t *pd, uint32_t pd_index,
                                   uint32_t pt_index, uint32_t page_count,
                                   paddr_t pa, uint32_t flags, vaddr_t va) {
  if (!ensure_page_table_exists(pd, pd_index, flags)) {
    return;
  }

  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  uint32_t *pt_virt = get_pt_virtual(pt_phys);

  fill_page_table_entries(pt_virt, pt_index, page_count, pa, flags);

  // Flush individual pages for partial mappings
  for (uint32_t i = 0; i < page_count; i++) {
    tlb_flush(va + i * PAGE_SIZE);
  }
}

//
// Page Table Unmapping Operations
//

static void clear_page_table_entries(uint32_t *pt, uint32_t start_index,
                                     uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    if (pt[start_index + i] & PD_PT_PRESENT) {
      pt[start_index + i] = 0;
    }
  }
}

static void unmap_full_page_table(page_dir_t *pd, uint32_t pd_index) {
  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  pmm_frame_free(pt_phys);
  pd[pd_index] = 0;
}

static void unmap_partial_page_table(page_dir_t *pd, uint32_t pd_index,
                                     uint32_t pt_index, uint32_t page_count) {
  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  uint32_t *pt_virt = get_pt_virtual(pt_phys);
  clear_page_table_entries(pt_virt, pt_index, page_count);
}

//
// Range Mapping/Unmapping Logic
//

static uint32_t calculate_map_size(uint32_t pt_index, vaddr_t va,
                                   vaddr_t va_end) {
  uint32_t remaining_in_pt = calc_remaining_in_pt(pt_index);
  uint32_t remaining_total = va_end - va;
  return (remaining_in_pt < remaining_total) ? remaining_in_pt
                                             : remaining_total;
}

bool hal_vm_map_range(arch_vm_t *vm, paddr_t pa_start, vaddr_t va_start,
                      size_t size, uint32_t flags) {

  page_dir_t *pd_virt = vm->pd;

  if (size == 0) {
    return true;
  }

  ASSERT(is_aligned(va_start, PAGE_SIZE));
  ASSERT(is_aligned(pa_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  vaddr_t va = va_start;
  paddr_t pa = pa_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = get_pd_index(va);
    uint32_t pt_index = get_pt_index(va);
    uint32_t map_size = calculate_map_size(pt_index, va, va_end);

    if ((pt_index == 0 && map_size >= PT_SIZE)) {
      // Fast path: map entire 4MB region
      map_full_page_table(pd_virt, pd_index, pa, flags);
      va += PT_SIZE;
      pa += PT_SIZE;
    } else {
      // Slow path: map partial page table
      uint32_t page_count = map_size / PAGE_SIZE;
      map_partial_page_table(pd_virt, pd_index, pt_index, page_count, pa, flags,
                             va);
      va += map_size;
      pa += map_size;
    }
  }

  if (size > PAGE_SIZE) {
    tlb_flush_all();
  }

  return true;
}

bool hal_vm_unmap_range(arch_vm_t *vm, vaddr_t va_start, size_t size) {
  if (size == 0) {
    return true;
  }
  page_dir_t *pd_virt = vm->pd;

  ASSERT(is_aligned(va_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  vaddr_t va = va_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = get_pd_index(va);
    uint32_t pt_index = get_pt_index(va);

    if (!(pd_virt[pd_index] & PD_PT_PRESENT)) {
      // Skip to next page table boundary
      va = (pd_index + 1) << 22;
      continue;
    }

    uint32_t unmap_size = calculate_map_size(pt_index, va, va_end);

    if ((pt_index == 0 && unmap_size >= PT_SIZE)) {
      unmap_full_page_table(pd_virt, pd_index);
      va += PT_SIZE;
    } else {
      uint32_t page_count = unmap_size / PAGE_SIZE;
      unmap_partial_page_table(pd_virt, pd_index, pt_index, page_count);
      va += unmap_size;
    }
  }

  if (size > PAGE_SIZE) {
    tlb_flush_all();
  }

  return true;
}

//
// Single Page Operations
//
bool hal_vm_map(arch_vm_t *vm, vaddr_t va, paddr_t pa, uint32_t flags) {
  bool res = hal_vm_map_range(vm, pa, va, PAGE_SIZE, flags);
  if (res) {
    tlb_flush(va);
  }
  return res;
}

bool hal_vm_unmap(arch_vm_t *vm, vaddr_t virt_addr) {
  bool res = hal_vm_unmap_range(vm, virt_addr, PAGE_SIZE);
  if (res) {
    tlb_flush(virt_addr);
  }
  return res;
}

static arch_vm_t k_arch_ctx;

arch_vm_t *hal_vm_context_create() {
  // 1. Allocate the container struct
  // If kmalloc isn't ready, we use our static kernel container
  static bool k_ctx_used = false;
  arch_vm_t *ctx;

  if (!k_ctx_used) {
    ctx = &k_arch_ctx;
    k_ctx_used = true;
  } else {
    ctx = kmalloc(sizeof(arch_vm_t));
  }

  if (!ctx)
    return NULL;

  // 2. Allocate the actual Physical Page Directory (1 frame)
  paddr_t pd_phys = pmm_frame_alloc();
  if (!pd_phys)
    return NULL;

  ctx->pd_phys = pd_phys;

  // 3. Map the PD into virtual memory so we can zero it
  // Use KERNEL_BASE if you are in a higher-half kernel
  ctx->pd = (uint32_t *)(pd_phys + KERNEL_BASE);
  memset(ctx->pd, 0, 4096);

  return ctx;
}

arch_vm_t *hal_vm_context_clone_kernel() {
  // This is called for new processes. kmalloc IS ready now.
  arch_vm_t *new_ctx = kmalloc(sizeof(arch_vm_t));
  if (!new_ctx)
    return NULL;

  paddr_t pd_phys = pmm_frame_alloc();
  if (!pd_phys) {
    kfree(new_ctx);
    return NULL;
  }

  new_ctx->pd_phys = pd_phys;
  new_ctx->pd = (uint32_t *)(pd_phys + KERNEL_BASE);

  // Copy the top 512 entries (2GB to 4GB)
  for (int i = 512; i < 1024; i++) {
    new_ctx->pd[i] = k_arch_ctx.pd[i];
  }

  return new_ctx;
}

void hal_vm_load_context(arch_vm_t *ctx) {
  if (!ctx)
    return;
  // The actual hardware switch
  asm volatile("mov %0, %%cr3" ::"r"(ctx->pd_phys));
}

void hal_vm_context_destroy(arch_vm_t *ctx) {
  if (!ctx)
    return;

  // Never destroy the static kernel context
  if (ctx == &k_arch_ctx)
    return;

  // Free the physical frame of the PD
  pmm_frame_free(ctx->pd_phys);

  // Free the container
  kfree(ctx);
}
