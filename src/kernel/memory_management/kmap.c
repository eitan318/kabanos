#include "kmap.h"
#include "hal.h"
#include "memory_management/memdefs.h"
#include "memory_management/vmspace.h"
#include "stdio.h"

static vaddr_t kmap_temp = KMAPPING_BASE;
extern vmspace_t *g_kernel_vmspace;

void *kmap(uint32_t phys_addr) {
  // Map physical page into kernel PD
  if (!hal_vm_map(g_kernel_vmspace->arch, kmap_temp, phys_addr,
                  PAGE_READWRITE)) {
    kdebugf("kmap: failed to map phys 0x%x\n", phys_addr);
    return NULL;
  }

  return (void *)kmap_temp;
}

void kunmap(void) {
  // Unmap the page
  hal_vm_unmap(g_kernel_vmspace->arch, kmap_temp);
}
