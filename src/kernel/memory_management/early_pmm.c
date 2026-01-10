#include "early_pmm.h"
#include "memory_management/memdefs.h"
#include "memory_management/vmm.h"

extern uint8_t _kernel_end;

static uintptr_t mem_start;
static uintptr_t mem_head;

void early_mem_init(void) {
  mem_start = ((uintptr_t)&_kernel_end - KERNEL_BASE);
  mem_head = mem_start;
}

void *early_pmm_alloc(size_t size) {
  if (mem_head + size - mem_start >= EARLY_PMM_SIZE) {
    return 0;
  }
  void *addr = (void *)mem_head;
  mem_head += align_up(size, 8);
  return addr;
}

void *early_pmm_vm_alloc(size_t size) {
  return early_pmm_alloc(size) + KERNEL_BASE;
}
