/**
 * @file vmspace.c
 * @brief Address-space lifecycle, vma tracking and cross-space copies.
 *
 * Cross-space copies use the single-page KMAPPING_BASE scratch window to
 * reach the other space's physical pages, so they are serialized by a
 * lock.
 */
#include "mm/vmspace.h"
#include "adt/range.h"
#include "arch/types.h"
#include "assert.h"
#include "hal.h"
#include "klib/errno.h"
#include "klib/stddef.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/va_allocation.h"
#include "proc/proc.h"
#include "sched/dispatcher.h"
#include "utils/math.h"

static arch_vm_t kernel_arch_vm;

void kernel_vmspace_create(vmspace_t *vmspace, range_t total_memory_range) {
  hal_vm_empty_arch_vm_create(&kernel_arch_vm);
  vmspace->arch = &kernel_arch_vm;
  vmspace->vma_list = NULL;

  // Map to HIGHER HALF ONLY
  hal_vm_map_range(vmspace->arch, total_memory_range.start,
                   total_memory_range.start + KERNEL_BASE,
                   total_memory_range.end, PAGE_READWRITE);

  // Map VGA buffer BEFORE switching
  hal_vm_map(vmspace->arch, VGA_SCREEN_BUF, VGA_SCREEN_BUF_PHYS,
             PAGE_READWRITE);
}

// Create virtual memory space for user processes
vmspace_t *vmspace_create() {
  vmspace_t *vmspace = kmalloc(sizeof(*vmspace));
  if (!vmspace)
    return NULL;
  vmspace->arch = kmalloc(sizeof(*vmspace->arch));
  bool res = hal_vm_empty_arch_vm_create(vmspace->arch);
  if (!res) {
    kfree(vmspace->arch);
    kfree(vmspace);
    return NULL;
  }

  extern vmspace_t *g_kernel_vmspace;
  hal_vm_arch_clone_mapping(vmspace->arch, g_kernel_vmspace->arch);

  vmspace->vma_list = NULL;

  return vmspace;
}

void vmspace_switch(vmspace_t *vmspace) {
  ASSERT(vmspace);
  hal_vm_arch_load(vmspace->arch);
}

vmspace_t *vmspace_clone(vmspace_t *original) {
  ASSERT(original);
  vmspace_t *vmspace_clone = vmspace_create();

  // clone vma list
  vma_t *curr_orig_vma = original->vma_list;
  vma_t *prev_new_vma = NULL;
  while (curr_orig_vma) {
    vma_t *new_vma =
        vma_create(curr_orig_vma->range.start,
                   curr_orig_vma->range.end - curr_orig_vma->range.start,
                   curr_orig_vma->flags);

    if (prev_new_vma) {
      prev_new_vma->next = new_vma;
    } else {
      vmspace_clone->vma_list = new_vma;
    }
    prev_new_vma = new_vma;
    curr_orig_vma = curr_orig_vma->next;
  }

  // clone arch
  hal_vm_arch_clone(vmspace_clone->arch, original->arch);
  return vmspace_clone;
}

void vmspace_destroy(vmspace_t *vmspace) {
  ASSERT(vmspace);

  extern vmspace_t *g_kernel_vmspace;

  // kernel vmspace shall not be freed because it is early-kernel-allocated
  if (vmspace->arch == g_kernel_vmspace->arch)
    return;

  while (vmspace->vma_list) {
    vma_t *temp = vmspace->vma_list;
    vmspace->vma_list = vmspace->vma_list->next;
    kfree(temp);
  }

  hal_vm_arch_destroy(vmspace->arch);
  kfree(vmspace->arch);
  kfree(vmspace);
}

void vmspace_add_vma(vmspace_t *vmspace, vma_t *new_vma) {
  ASSERT(vmspace && new_vma);

  new_vma->next = vmspace->vma_list;
  vmspace->vma_list = new_vma;
}

vma_t *vmspace_find_vma(vmspace_t *vmspace, vaddr_t addr) {
  ASSERT(vmspace);

  // Start at the head of the list
  vma_t *curr = vmspace->vma_list;

  while (curr) {
    if (addr >= curr->range.start && addr < curr->range.end) {
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

vma_t *vma_create(vaddr_t va_start, size_t mem_size, uint32_t flags) {
  vma_t *vma = kmalloc(sizeof(vma_t));
  if (!vma)
    return NULL;

  vma->range.start = va_start;
  vma->range.end = va_start + mem_size;
  vma->flags = flags;
  vma->next = NULL;

  return vma;
}

bool vmspace_map_stack(vmspace_t *vm, uint32_t stack_top, size_t size) {
  ASSERT(is_aligned(stack_top, PAGE_SIZE));
  ASSERT(is_aligned(size, PAGE_SIZE));
  uint32_t stack_start = stack_top - size;

  vma_t *stack_vma = vma_create(stack_start, size,
                                VMA_READ | VMA_WRITE | VMA_USER | VMA_STACK);
  if (!stack_vma)
    return false;

  vmspace_add_vma(vm, stack_vma);

  // Allocate eagerly; returning true instead would defer to demand paging
  uint32_t flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
  return va_alloc_region(vm->arch, stack_start, size, flags);
}

bool vmspace_map_heap(vmspace_t *vm, uint32_t heap_start, size_t initial_size) {
  ASSERT(is_aligned(heap_start, PAGE_SIZE));
  ASSERT(is_aligned(initial_size, PAGE_SIZE));

  vma_t *heap_vma = vma_create(heap_start, initial_size,
                               VMA_READ | VMA_WRITE | VMA_USER | VMA_HEAP);
  if (!heap_vma)
    return false;

  vmspace_add_vma(vm, heap_vma);

  uint32_t flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
  return va_alloc_region(vm->arch, heap_start, initial_size, flags);
}

bool vmspace_extend_heap(vmspace_t *vm, uint32_t old_brk, uint32_t new_brk) {
  ASSERT(is_aligned(old_brk, PAGE_SIZE));
  ASSERT(is_aligned(new_brk, PAGE_SIZE));

  if (new_brk <= old_brk)
    return true; // shrink not supported yet

  // Find the heap VMA and extend it
  vma_t *heap_vma = NULL;
  vma_t *curr = vm->vma_list;
  while (curr) {
    if (curr->flags & VMA_HEAP) {
      heap_vma = curr;
      break;
    }
    curr = curr->next;
  }

  if (!heap_vma)
    return false;

  size_t extra = new_brk - old_brk;
  uint32_t flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;

  if (!va_alloc_region(vm->arch, old_brk, extra, flags))
    return false;

  heap_vma->range.end = new_brk;
  return true;
}

static spinlock_t kmpping_lock;

int vmspace_copy_from(arch_vm_t *src_vmspace, vaddr_t dst_vaddr,
                      vaddr_t src_vaddr, size_t size) {
  if (!src_vmspace)
    return -EINVAL;

  arch_vm_t *curr_vmspace = dispatch_get_current()->process->vmspace->arch;
  size_t remaining = size;

  spinlock_acquire(&kmpping_lock);

  while (remaining > 0) {
    vaddr_t page_base = src_vaddr & ~((vaddr_t)PAGE_SIZE - 1);
    uint32_t offset = src_vaddr % PAGE_SIZE; // Offset must follow the source!
    size_t to_copy = PAGE_SIZE - offset;
    if (to_copy > remaining)
      to_copy = remaining;

    paddr_t src_phys = hal_vm_virt_to_phys(src_vmspace, page_base);
    if (src_phys == 0) {
      spinlock_release(&kmpping_lock);
      return -EFAULT;
    }

    hal_vm_map(curr_vmspace, KMAPPING_BASE, src_phys, PAGE_READWRITE);
    memcpy((void *)dst_vaddr, (void *)(KMAPPING_BASE + offset), to_copy);
    hal_vm_unmap(curr_vmspace, KMAPPING_BASE);

    remaining -= to_copy;
    dst_vaddr += to_copy;
    src_vaddr += to_copy;
  }

  spinlock_release(&kmpping_lock);
  return 0;
}

int vmspace_copy_to(arch_vm_t *dst_vmspace, vaddr_t dst_vaddr,
                    vaddr_t src_vaddr, size_t size) {
  if (!dst_vmspace)
    return -EINVAL;

  arch_vm_t *curr_vmspace = dispatch_get_current()->process->vmspace->arch;
  size_t remaining = size;

  spinlock_acquire(&kmpping_lock);

  while (remaining > 0) {
    vaddr_t page_base = dst_vaddr & ~((vaddr_t)PAGE_SIZE - 1);
    uint32_t offset = dst_vaddr % PAGE_SIZE;
    size_t to_copy = PAGE_SIZE - offset;
    if (to_copy > remaining)
      to_copy = remaining;

    paddr_t dst_phys = hal_vm_virt_to_phys(dst_vmspace, page_base);
    if (dst_phys == 0) {
      spinlock_release(&kmpping_lock);
      return -EFAULT;
    }

    hal_vm_map(curr_vmspace, KMAPPING_BASE, dst_phys, PAGE_READWRITE);

    // Copy FROM current vmspace (src) TO the mapped window (dst)
    memcpy((void *)(KMAPPING_BASE + offset), (void *)src_vaddr, to_copy);

    hal_vm_unmap(curr_vmspace, KMAPPING_BASE);

    remaining -= to_copy;
    dst_vaddr += to_copy;
    src_vaddr += to_copy;
  }

  spinlock_release(&kmpping_lock);
  return 0;
}
