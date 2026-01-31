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
  // Copy kernel mappings while we're still in kernel's CR3
  extern vmspace_t *g_kernel_vmspace;
  memcpy(vmspace->pd, g_kernel_vmspace->pd, PAGE_SIZE);

  vmspace->pd = temp_pd;
  return vmspace;
}

void vmspace_switch(vmspace_t *vmspace) {
  asm volatile("mov %0, %%cr3" ::"r"(vmspace->pd_phys));
}

void vmspace_destroy(vmspace_t *vmspace) {
  if (!vmspace)
    return;
  extern vmspace_t *g_kernel_vmspace;

  // kernel vmspace shall not be freed because it is early-kernel-allocated
  if (vmspace->pd == g_kernel_vmspace->pd)
    return;

  pmm_frame_free(vmspace->pd_phys);
  kfree(vmspace);
}
