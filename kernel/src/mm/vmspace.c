#include "mm/vmspace.h"
#include "adt/range.h"
#include "arch/types.h"
#include "assert.h"
#include "hal.h"
#include "klib/stddef.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/va_allocation.h"
#include "utils/math.h"
#include <stdarg.h>

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
  vma->next = NULL; // Safety first

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

  // 3. EITHER allocate now:
  uint32_t flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
  return va_alloc_region(vm->arch, stack_start, size, flags);

  // OR just return true here if you want lazy demand paging!
  return true;
}
