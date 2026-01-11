#include "early_pmm.h"
#include "memory_management/memdefs.h"
#include "memory_management/vmm.h"
#include "utils/math.h"

static uintptr_t mem_start;
static uintptr_t mem_head;
static bool enabled;

void early_pmm_init(uintptr_t start_phys_addr) {
  enabled = true;
  mem_start = start_phys_addr - KERNEL_BASE;
  mem_head = mem_start;
}

void early_pmm_disable() { enabled = false; }

static void *early_pmm_alloc(size_t size) {
  if (!enabled || mem_head + size - mem_start >= EARLY_PMM_SIZE) {
    return NULL;
  }
  void *addr = (void *)mem_head;
  mem_head += align_up(size, 8);
  return addr;
}

void *early_pmm_vm_alloc(size_t size) {
  return early_pmm_alloc(size) + KERNEL_BASE;
}
