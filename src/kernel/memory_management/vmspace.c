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

// Create initial virtual memory space for kernel
void kernel_vmspace_creat(vmspace_t *vmspace) {
  paddr_t pd_phys = (paddr_t)pmm_frame_alloc();
  if (!pd_phys)
    return;
  // TODO: make virt
  vmspace->pd = (uint32_t *)pd_phys;
  vmspace->pd_phys = pd_phys;
  memset(vmspace->pd, 0, PAGE_SIZE);

  // map kernel code/data/bss
  extern Range g_kernel_phys_range;
  uint32_t initial_range_start = 0;
  uint32_t initial_range_size = align_up(g_kernel_phys_range.end, PAGE_SIZE) +
                                align_up(EARLY_PMM_SIZE, PAGE_SIZE) -
                                initial_range_start;

  // identity map kernel code and early pmm to heigher helf and lower half
  map_range(vmspace->pd, initial_range_start, initial_range_start,
            initial_range_size, PAGE_READWRITE);

  map_range(vmspace->pd, initial_range_start, initial_range_start + KERNEL_BASE,
            initial_range_size, PAGE_READWRITE);
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
