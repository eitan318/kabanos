#include "early_pmm.h"

extern uint8_t _kernel_end;

static uintptr_t mem_start;
static uintptr_t mem_head;

void early_mem_init(void) {
  mem_start = (uintptr_t)&_kernel_end;
  mem_head = mem_start;
}

void *early_pmm_alloc(size_t size) {
  if (mem_head + size - mem_start >= EARLY_IDENTITY_MAP_SIZE) {
    return 0;
  }
  void *addr = (void *)mem_head;
  mem_head += align_up(size, 8);
  return addr;
}

Range early_pmm_get_used_range() { return (Range){mem_start, mem_head}; }
