#pragma once
#include "hal.h"
#include "utils/range.h"
#include <stdint.h>

typedef struct vmspace_t {
  uint32_t *pd;
  paddr_t pd_phys;
} vmspace_t;

// Create initial virtual memory space for kernel
void kernel_vmspace_create(vmspace_t *vmspace, Range total_memory_range);
// Create virtual memory space for user processes
vmspace_t *vmspace_create();

void vmspace_switch(vmspace_t *vmspace);
void vmspace_destroy(vmspace_t *vmspace);
