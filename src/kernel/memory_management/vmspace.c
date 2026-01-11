#include "vmspace.h"
#include "include/memory.h"
#include "kmalloc.h"
#include "memory_management/early_pmm.h"
#include "memory_management/memdefs.h"
#include "utils/math.h"
#include "utils/range.h"
#include "vmm.h"
#include <stddef.h>

// Map physical range to virtual range
static void map_range(page_dir_t *pd_virt, paddr_t pa_start, vaddr_t va_start,
                      size_t size, uint32_t flags) {
  pa_start = align_down(pa_start, PAGE_SIZE);
  va_start = align_down(va_start, PAGE_SIZE);
  for (size_t offset = 0; offset < size; offset += PAGE_SIZE) {
    vm_map(pd_virt, va_start + offset, pa_start + offset, flags);
  }
}

void kernel_vmspace_create(vmspace_t *vmspace) {
  paddr_t pd_phys = (paddr_t)pmm_frame_alloc();
  if (!pd_phys)
    return;

  vmspace->pd = (uint32_t *)pd_phys;
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  extern Range g_kernel_phys_range;

  // Calculate the kernel's physical range including early PMM
  uint32_t kernel_start_phys = 0;
  uint32_t kernel_end_phys = align_up(g_kernel_phys_range.end, PAGE_SIZE);
  uint32_t early_pmm_end =
      kernel_end_phys + align_up(EARLY_PMM_SIZE, PAGE_SIZE);
  uint32_t total_size = early_pmm_end - kernel_start_phys;

  // Map to HIGHER HALF ONLY
  map_range(vmspace->pd, kernel_start_phys, kernel_start_phys + KERNEL_BASE,
            total_size, PAGE_READWRITE);

  // Map VGA buffer BEFORE switching (critical for debugging)
  vm_map(vmspace->pd, VGA_SCREEN_BUF, VGA_SCREEN_BUF_PHYS, PAGE_READWRITE);
}

// Create virtual memory space for user processes
vmspace_t *user_vmspace_creat() {
  vmspace_t *vmspace = kmalloc(sizeof(*vmspace));
  if (!vmspace) {
    return NULL;
  }
  // TODO: check if the frame allocator allocating from all avail memory range
  // is problematic
  paddr_t pd_phys = pmm_frame_alloc();
  if (!pd_phys) {
    kfree(vmspace);
    return NULL;
  }

  vmspace->pd = (uint32_t *)pd_phys;
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  // Copy kernel mappings (including identity map)
  extern vmspace_t *g_kernel_vmspace;
  memcpy(vmspace->pd, g_kernel_vmspace->pd, PAGE_SIZE);

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
