#include "vmspace.h"
#include "hal.h"
#include "kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/pmm.h"
#include "string.h"
#include "utils/range.h"
#include <stddef.h>

void kernel_vmspace_create(vmspace_t *vmspace, Range total_memory_range) {
  vmspace->pd_phys = vm_empty_pd_create();
  if (!vmspace->pd_phys)
    return;
  vmspace->pd = (uint32_t *)vmspace->pd_phys;

  // Map to HIGHER HALF ONLY
  vm_map_range(vmspace->pd, total_memory_range.start,
               total_memory_range.start + KERNEL_BASE, total_memory_range.end,
               PAGE_READWRITE);

  // Map VGA buffer BEFORE switching
  vm_map(vmspace->pd, VGA_SCREEN_BUF, VGA_SCREEN_BUF_PHYS, PAGE_READWRITE);
  vmspace->pd = (uint32_t *)(vmspace->pd_phys + KERNEL_BASE);
}

// Create virtual memory space for user processes
vmspace_t *vmspace_create() {
  vmspace_t *vmspace = kmalloc(sizeof(*vmspace));
  if (!vmspace)
    return NULL;

  vmspace->pd_phys = vm_empty_pd_create();
  if (!vmspace->pd_phys) {
    kfree(vmspace);
    return NULL;
  }

  // Temporarily map the new PD in current address space to initialize it
  uint32_t *temp_pd = (uint32_t *)(vmspace->pd_phys + KERNEL_BASE);

  // Copy kernel mappings while we're still in kernel's CR3
  extern vmspace_t *g_kernel_vmspace;
  memcpy(temp_pd, g_kernel_vmspace->pd, PAGE_SIZE);

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
