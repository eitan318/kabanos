#include "vmspace.h"
#include "arch/types.h"
#include "hal.h"
#include "kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "string.h"
#include "utils/range.h"
#include <stddef.h>

static arch_vm_t kernel_arch_vm;

void kernel_vmspace_create(vmspace_t *vmspace, Range total_memory_range) {
  hal_vm_empty_arch_vm_create(&kernel_arch_vm);
  vmspace->arch = &kernel_arch_vm;

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
  hal_vm_arch_clone(vmspace->arch, g_kernel_vmspace->arch);

  return vmspace;
}

void vmspace_switch(vmspace_t *vmspace) { hal_vm_arch_load(vmspace->arch); }

vmspace_t *vmspace_clone(vmspace_t *original) {
  vmspace_t *vmspace_clone = vmspace_create();
  /// hial_vm_arch_clone(vmspace_clone->arch, original->arch);
  return vmspace_clone;
}

void vmspace_destroy(vmspace_t *vmspace) {
  if (!vmspace)
    return;
  extern vmspace_t *g_kernel_vmspace;

  // kernel vmspace shall not be freed because it is early-kernel-allocated
  if (vmspace->arch == g_kernel_vmspace->arch)
    return;

  hal_vm_arch_destroy(vmspace->arch);
  kfree(vmspace->arch);
  kfree(vmspace);
}
