#include "boot_allocator.h"
extern uint8_t _kernel_end;
static uintptr_t placement = (uintptr_t)&_kernel_end;

void *boot_alloc(size_t size) {
  void *addr = (void *)placement;
  placement += align_up(size, 8);
  return addr;
}

Range boot_alloc_get_used_range() {
  return (Range){(uintptr_t)&_kernel_end, placement};
}
