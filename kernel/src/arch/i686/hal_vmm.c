#include "arch/types.h"
#include "assert.h"
#include "hal.h"
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdio.h"
#include "mm/memdefs.h"
#include "mm/pmm.h"
#include "proc/proc.h"
#include "utils/math.h"
#include <sched/dispatcher.h>

typedef uint32_t page_dir_entry_t;

#define KERNEL_PD_START (KERNEL_BASE / (4 * 1024 * 1024))
#define ARCH_PD_PT_PRESENT 0x1
#define ARCH_PD_PT_READWRITE 0x2
#define ARCH_PD_PT_USER 0x4
#define PT_ENTRIES 1024
#define PD_ENTRIES PAGE_SIZE / sizeof(uint32_t)
#define PT_SIZE (PT_ENTRIES * PAGE_SIZE)

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

static inline uint32_t calc_remaining_in_pt(uint32_t pt_index) {
  return (PT_ENTRIES - pt_index) * PAGE_SIZE;
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

static bool ensure_page_table_exists(page_dir_entry_t *pd, uint32_t pd_index,
                                     uint32_t internal_flags) {
  if (pd[pd_index] & ARCH_PD_PT_PRESENT) {
    return true;
  }

  paddr_t pt_phys = allocate_page_table();
  if (!pt_phys) {
    return false;
  }

  pd[pd_index] = pt_phys | ARCH_PD_PT_PRESENT | ARCH_PD_PT_READWRITE |
                 (internal_flags & ARCH_PD_PT_USER);
  return true;
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

static void map_full_page_table(page_dir_entry_t *pd, uint32_t pd_index,
                                paddr_t pa, uint32_t arch_flags) {
  bool created;
  paddr_t pt_phys =
      get_or_create_page_table(pd, pd_index, arch_flags, &created);
  uint32_t *pt_virt = get_pt_virtual(pt_phys);

  fill_page_table_entries(pt_virt, 0, PT_ENTRIES, pa, arch_flags);
}

static void map_partial_page_table(page_dir_entry_t *pd, uint32_t pd_index,
                                   uint32_t pt_index, uint32_t page_count,
                                   paddr_t pa, uint32_t arch_flags,
                                   vaddr_t va) {
  if (!ensure_page_table_exists(pd, pd_index, arch_flags)) {
    return;
  }

  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  uint32_t *pt_virt = get_pt_virtual(pt_phys);

  fill_page_table_entries(pt_virt, pt_index, page_count, pa, arch_flags);

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
    if (pt[start_index + i] & ARCH_PD_PT_PRESENT) {
      pt[start_index + i] = 0;
    }
  }
}

static void unmap_full_page_table(page_dir_entry_t *pd, uint32_t pd_index) {
  paddr_t pt_phys = pd[pd_index] & ~0xFFF;
  pmm_frame_free(pt_phys);
  pd[pd_index] = 0;
}

static void unmap_partial_page_table(page_dir_entry_t *pd, uint32_t pd_index,
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
                      size_t size, uint32_t public_flags) {

  uint32_t arch_flags = public_flags_to_arch_flags(public_flags);
  page_dir_entry_t *pd_virt = vm->pd;

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
      map_full_page_table(pd_virt, pd_index, pa, arch_flags);
      va += PT_SIZE;
      pa += PT_SIZE;
    } else {
      // Slow path: map partial page table
      uint32_t page_count = map_size / PAGE_SIZE;
      map_partial_page_table(pd_virt, pd_index, pt_index, page_count, pa,
                             arch_flags, va);
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
  page_dir_entry_t *pd_virt = vm->pd;

  ASSERT(is_aligned(va_start, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));

  vaddr_t va = va_start;
  vaddr_t va_end = va_start + size;

  while (va < va_end) {
    uint32_t pd_index = get_pd_index(va);
    uint32_t pt_index = get_pt_index(va);

    if (!(pd_virt[pd_index] & ARCH_PD_PT_PRESENT)) {
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
bool hal_vm_map(arch_vm_t *vm, vaddr_t va, paddr_t pa, uint32_t public_flags) {
  bool res = hal_vm_map_range(vm, pa, va, PAGE_SIZE, public_flags);
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

bool hal_vm_empty_arch_vm_create(arch_vm_t *kernel_arch_vm) {
  kernel_arch_vm->pd_phys = allocate_page_table();
  if (!kernel_arch_vm->pd_phys)
    return false;
  kernel_arch_vm->pd = (uint32_t *)(kernel_arch_vm->pd_phys + KERNEL_BASE);
  return true;
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

int hal_vm_copy_to_vmspace(arch_vm_t *dst_vmspace, vaddr_t dst_vaddr,
                           vaddr_t src_vaddr, size_t size) {
  arch_vm_t *curr_vmspace = dispatch_get_current()->process->vmspace->arch;
  size_t remaining = size;
  vaddr_t curr_dst = dst_vaddr;
  vaddr_t curr_src = src_vaddr;

  while (remaining > 0) {
    uint32_t offset = curr_dst % PAGE_SIZE;

    // 2. Determine how much we can copy in THIS page
    size_t to_copy = PAGE_SIZE - offset;
    if (to_copy > remaining) {
      to_copy = remaining;
    }
    paddr_t dst_phys_page = hal_vm_virt_to_phys(dst_vmspace, curr_dst - offset);
    hal_vm_map(curr_vmspace, KMAPPING_BASE, dst_phys_page, PAGE_READWRITE);
    memcpy((void *)(KMAPPING_BASE + offset), (void *)curr_src, to_copy);
    hal_vm_unmap(curr_vmspace, KMAPPING_BASE);

    remaining -= to_copy;
    curr_dst += to_copy;
    curr_src += to_copy;
  }

  return 0;
}

int hal_vm_copy_from_vmspace(arch_vm_t *src_vmspace, vaddr_t dst_vaddr,
                             vaddr_t src_vaddr, size_t size) {
  arch_vm_t *curr_vmspace = dispatch_get_current()->process->vmspace->arch;
  size_t remaining = size;
  vaddr_t curr_dst = dst_vaddr;
  vaddr_t curr_src = src_vaddr;

  while (remaining > 0) {
    uint32_t offset = dst_vaddr % PAGE_SIZE;
  }

  // TODO: continue
  return -1;
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
