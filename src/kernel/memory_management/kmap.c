#include "kmap.h"
#include "memory_management/memdefs.h"
#include "memory_management/vmm.h"
#include "memory_management/vmspace.h"
#include "stdio.h"

static vaddr_t kmap_temp = KMAPPING_BASE;
extern vmspace_t *g_kernel_vmspace;

void *kmap(uint32_t phys_addr) {
  // Map physical page into kernel PD
  if (!vm_map(g_kernel_vmspace->pd, kmap_temp, phys_addr, PAGE_READWRITE)) {
    debugf("kmap: failed to map phys 0x%x\n", phys_addr);
    return NULL;
  }

  return (void *)kmap_temp;
}

void kunmap(void) {
  // Unmap the page
  vm_unmap(g_kernel_vmspace->pd, kmap_temp);
}
