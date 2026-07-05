/**
 * @file hal_vmm.c
 * @brief i686 two-level paging: page directory/table management and COW.
 *
 * Page tables are accessed through the higher-half physical mapping
 * (phys + KERNEL_BASE). COW pages are marked read-only with the custom
 * PT_COW software bit; the write fault handler re-links or copies them.
 */
#include "arch/types.h"
#include "assert.h"
#include "hal.h"
#include "klib/errno.h"
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdio.h"
#include "mm/memdefs.h"
#include "mm/pmm.h"
#include "panic.h"
#include "utils/math.h"
#include <sched/dispatcher.h>
#include <stdint.h>

typedef uint32_t page_dir_entry_t;

#define KERNEL_PD_START (KERNEL_BASE / (4 * 1024 * 1024))
#define ARCH_PD_PT_PRESENT 0x1
#define ARCH_PD_PT_READWRITE 0x2
#define ARCH_PD_PT_USER 0x4
#define PT_ENTRIES 1024
#define PD_ENTRIES PAGE_SIZE / sizeof(uint32_t)

#define PT_COW 0x200 // Bit 9: Our custom "Copy-on-Write" flag

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

static inline uint32_t pt_pages_remaining(uint32_t pt_index) {
  return PT_ENTRIES - pt_index;
}

static uint32_t public_flags_to_arch_flags(uint32_t flags) {
  uint32_t arch_flags = 0;
  if (flags & PAGE_PRESENT)
    arch_flags |= ARCH_PD_PT_PRESENT;
  if (flags & PAGE_READWRITE)
    arch_flags |= ARCH_PD_PT_READWRITE;
  if (flags & PAGE_USER)
    arch_flags |= ARCH_PD_PT_USER;

  return arch_flags;
}

paddr_t hal_vm_virt_to_phys(arch_vm_t *vm, vaddr_t va) {
  page_dir_entry_t *pd = vm->pd;
  uint32_t pd_index = get_pd_index(va);
  uint32_t pt_index = get_pt_index(va);

  if (!(pd[pd_index] & ARCH_PD_PT_PRESENT)) {
    return 0;
  }

  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  uint32_t *pt_virt = get_pt_virtual(pt_phys);

  if (!(pt_virt[pt_index] & ARCH_PD_PT_PRESENT)) {
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
    kdebugf("Alloc FAILED The allocator was out of memory!");
    return 0;
  }

  uint32_t *pt_virt = get_pt_virtual(pt_phys);
  memset(pt_virt, 0, PAGE_SIZE);
  return pt_phys;
}

static paddr_t get_or_create_page_table(page_dir_entry_t *pd, uint32_t pd_index,
                                        uint32_t arch_flags, bool *created) {
  if (pd[pd_index] & ARCH_PD_PT_PRESENT) {
    *created = false;
    return pd[pd_index] & ~0xFFF;
  }

  paddr_t pt_phys = allocate_page_table();
  if (!pt_phys) {
    return 0;
  }

  pd[pd_index] = pt_phys | ARCH_PD_PT_PRESENT | ARCH_PD_PT_READWRITE |
                 (arch_flags & ARCH_PD_PT_USER);
  *created = true;
  return pt_phys;
}

//
// Page Table Mapping Operations
//

static void fill_page_table_entries(uint32_t *pt, uint32_t start_index,
                                    uint32_t count, paddr_t pa_base,
                                    uint32_t arch_flags) {
  for (uint32_t i = 0; i < count; i++) {
    pt[start_index + i] =
        (pa_base + i * PAGE_SIZE) | (arch_flags & 0xFFF) | ARCH_PD_PT_PRESENT;
  }
}

/** @brief Maps @p page_count entries starting at @p pt_index, creating the
 *         page table if needed. Returns false only on allocation failure. */
static bool map_page_table_range(page_dir_entry_t *pd, uint32_t pd_index,
                                 uint32_t pt_index, uint32_t page_count,
                                 paddr_t pa, uint32_t arch_flags) {
  bool created;
  paddr_t pt_phys =
      get_or_create_page_table(pd, pd_index, arch_flags, &created);
  if (!pt_phys) {
    return false;
  }

  uint32_t *pt_virt = get_pt_virtual(pt_phys);
  fill_page_table_entries(pt_virt, pt_index, page_count, pa, arch_flags);
  return true;
}

//
// Page Table Unmapping Operations
//
static void clear_page_table_entries(uint32_t *pt, uint32_t start_index,
                                     uint32_t count) {
  for (uint32_t i = 0; i < count; i++) {
    if (pt[start_index + i] & ARCH_PD_PT_PRESENT) {
      pt[start_index + i] = 0;
    }
  }
}

/** @brief Clears @p page_count entries starting at @p pt_index. If that
 *         empties the entire table, reclaims it too -- except in kernel
 *         space, where page tables are preallocated once at boot and must
 *         stay put forever (every process PD shares them by reference). */
static void unmap_page_table_range(page_dir_entry_t *pd, uint32_t pd_index,
                                   uint32_t pt_index, uint32_t page_count) {
  if (!(pd[pd_index] & ARCH_PD_PT_PRESENT)) {
    return;
  }

  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  uint32_t *pt_virt = get_pt_virtual(pt_phys);
  clear_page_table_entries(pt_virt, pt_index, page_count);

  if (pt_index == 0 && page_count == PT_ENTRIES) {
    ASSERT(pd_index < KERNEL_PD_START);
    pmm_frame_free(pt_phys);
    pd[pd_index] = 0;
  }
}

//
// Range Mapping/Unmapping Logic
//
// Both loops walk the range one page-table's worth at a time (a chunk never
// crosses a 4MB boundary, so pd_index/pt_index stay valid for the whole
// chunk) and flush once at the end: a single invlpg for a lone page, a full
// TLB flush otherwise.
//

bool hal_vm_map_range(arch_vm_t *vm, paddr_t pa_start, vaddr_t va_start,
                      size_t size, uint32_t public_flags) {
  ASSERT(vm);

  if (size == 0) {
    return true;
  }

  ASSERT(is_aligned(va_start, PAGE_SIZE));
  ASSERT(is_aligned(pa_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  uint32_t arch_flags = public_flags_to_arch_flags(public_flags);
  page_dir_entry_t *pd_virt = vm->pd;

  vaddr_t va = va_start;
  paddr_t pa = pa_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = get_pd_index(va);
    uint32_t pt_index = get_pt_index(va);
    uint32_t page_count =
        MIN(pt_pages_remaining(pt_index), (va_end - va) / PAGE_SIZE);

    if (!map_page_table_range(pd_virt, pd_index, pt_index, page_count, pa,
                              arch_flags)) {
      return false;
    }

    va += page_count * PAGE_SIZE;
    pa += page_count * PAGE_SIZE;
  }

  if (size == PAGE_SIZE) {
    tlb_flush(va_start);
  } else {
    tlb_flush_all();
  }

  return true;
}

bool hal_vm_unmap_range(arch_vm_t *vm, vaddr_t va_start, size_t size) {
  ASSERT(vm);

  if (size == 0) {
    return true;
  }

  ASSERT(is_aligned(va_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  page_dir_entry_t *pd_virt = vm->pd;
  vaddr_t va = va_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = get_pd_index(va);
    uint32_t pt_index = get_pt_index(va);
    uint32_t page_count =
        MIN(pt_pages_remaining(pt_index), (va_end - va) / PAGE_SIZE);

    unmap_page_table_range(pd_virt, pd_index, pt_index, page_count);
    va += page_count * PAGE_SIZE;
  }

  if (size == PAGE_SIZE) {
    tlb_flush(va_start);
  } else {
    tlb_flush_all();
  }

  return true;
}

//
// Single Page Operations
//
bool hal_vm_map(arch_vm_t *vm, vaddr_t va, paddr_t pa, uint32_t public_flags) {
  return hal_vm_map_range(vm, pa, va, PAGE_SIZE, public_flags);
}

bool hal_vm_unmap(arch_vm_t *vm, vaddr_t virt_addr) {
  return hal_vm_unmap_range(vm, virt_addr, PAGE_SIZE);
}

bool hal_vm_empty_arch_vm_create(arch_vm_t *kernel_arch_vm) {
  kernel_arch_vm->pd_phys = allocate_page_table();
  if (!kernel_arch_vm->pd_phys)
    return false;
  kernel_arch_vm->pd = (uint32_t *)(kernel_arch_vm->pd_phys + KERNEL_BASE);
  return true;
}

/**
 * @brief Creates every kernel-half page table up front, once.
 *
 * Process page directories are created by copying the kernel's PDEs
 * (hal_vm_arch_clone_mapping) at process-creation time. If a kernel PDE were
 * created later (e.g. heap growth crossing a 4MB boundary), already-cloned
 * process PDs would never learn about it and would fault on that region
 * forever. Populating all 256 kernel PDEs before any process exists makes
 * every future kernel mapping just fill PTEs into an already-shared page
 * table, so no process PD can ever fall out of sync with the kernel's.
 *
 * Must run with the kernel address space already active (CR3 loaded):
 * creating a page table zeroes it through the physical-memory mapping,
 * which the bootstrap page tables from bringup only cover up to the end
 * of the kernel image. And must run before the first process is created.
 */
void hal_vm_prealloc_kernel_tables(arch_vm_t *vm) {
  for (uint32_t pd_index = KERNEL_PD_START; pd_index < PD_ENTRIES;
      pd_index++) {
    bool created;
    if (!get_or_create_page_table(vm->pd, pd_index, 0, &created)) {
      panic("hal_vm_prealloc_kernel_tables: out of memory");
    }
  }
}

void hal_vm_arch_load(arch_vm_t *arch_vm) {
  asm volatile("mov %0, %%cr3" ::"r"(arch_vm->pd_phys));
}

static uint32_t *get_pte_ptr(arch_vm_t *vm, vaddr_t va) {
  uint32_t pd_idx = get_pd_index(va);
  if (!(vm->pd[pd_idx] & ARCH_PD_PT_PRESENT))
    return NULL;

  uint32_t *pt_virt = get_pt_virtual(vm->pd[pd_idx] & ~0xFFF);
  return &pt_virt[get_pt_index(va)];
}

static void phys_copy(paddr_t dst_pa, paddr_t src_pa) {
  void *src = (void *)(src_pa + KERNEL_BASE);
  void *dst = (void *)(dst_pa + KERNEL_BASE);
  memcpy(dst, src, FRAME_SIZE);
}

bool hal_vmm_handle_cow(arch_vm_t *arch_vm, uintptr_t addr) {
  uint32_t *pte = get_pte_ptr(arch_vm, addr);

  if (!pte || !(*pte & PT_COW))
    return false;

  paddr_t old_phys = *pte & ~0xFFF;
  pmm_frame_refcount_inc(old_phys);
  if (pmm_frame_refcount_get(old_phys) > 1) {
    paddr_t new_phys = pmm_frame_alloc();

    phys_copy(new_phys, old_phys);

    *pte = new_phys | (*pte & 0xFFF) | ARCH_PD_PT_READWRITE;
    *pte &= ~PT_COW;

    pmm_frame_free(old_phys); // Decrements ref count
  } else {
    // Only one owner left, promote back to writable
    *pte |= ARCH_PD_PT_READWRITE;
    *pte &= ~PT_COW;
  }

  tlb_flush(addr);
  return true;
}

//
// Page Directory Lifetime
//

void hal_vm_arch_destroy(arch_vm_t *vm) {
  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (vm->pd[i] & ARCH_PD_PT_PRESENT) {
      paddr_t pt_phys = vm->pd[i] & ~0xFFF;
      pmm_frame_free(pt_phys);
    }
  }
  pmm_frame_free(vm->pd_phys);
}

static void hal_vmm_set_cow(arch_vm_t *arch_vm) {
  page_dir_entry_t *pd = arch_vm->pd;

  // Only iterate over USER space entries
  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (!(pd[i] & ARCH_PD_PT_PRESENT))
      continue;

    // 1. Convert physical address in PD to a virtual pointer we can use
    paddr_t pt_phys = pd[i] & ~0xFFF;
    uint32_t *pt = get_pt_virtual(pt_phys);

    for (int j = 0; j < PT_ENTRIES; j++) {
      if (!(pt[j] & ARCH_PD_PT_PRESENT))
        continue;

      // Only make WRITABLE pages COW
      if (pt[j] & ARCH_PD_PT_READWRITE) {
        pt[j] |= PT_COW;
        pt[j] &= ~ARCH_PD_PT_READWRITE; // Correct bitwise NOT
      }

      // Every page shared between parent and child needs +1 ref
      paddr_t page_phys = pt[j] & ~0xFFF;
      pmm_frame_refcount_inc(page_phys);
    }
  }
  tlb_flush_all();
}

void hal_vm_arch_clone_mapping(arch_vm_t *dst, arch_vm_t *src) {
  memcpy(dst->pd, src->pd, PAGE_SIZE);
}

void hal_vm_arch_clone(arch_vm_t *dst, arch_vm_t *src) {
  // Mark the source as COW first
  hal_vmm_set_cow(src);

  for (int i = 0; i < KERNEL_PD_START; i++) {
    if (src->pd[i] & ARCH_PD_PT_PRESENT) {
      paddr_t new_pt_phys = pmm_frame_alloc();
      uint32_t *new_pt_virt = get_pt_virtual(new_pt_phys);
      uint32_t *old_pt_virt = get_pt_virtual(src->pd[i] & ~0xFFF);

      memcpy(new_pt_virt, old_pt_virt, PAGE_SIZE);

      // Link the new PT into the child's PD
      dst->pd[i] = new_pt_phys | (src->pd[i] & 0xFFF);
    }
  }

  // Copy Kernel space entries (usually just a direct pointer copy as kernel is
  // shared)
  memcpy(&dst->pd[KERNEL_PD_START], &src->pd[KERNEL_PD_START],
         (1024 - KERNEL_PD_START) * sizeof(page_dir_entry_t));
}
