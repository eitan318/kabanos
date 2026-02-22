#pragma once
#include "adt/range.h"
#include "arch/types.h"
#include <stdint.h>

typedef struct vmspace_t {
  arch_vm_t *arch;
} vmspace_t;

// Create initial virtual memory space for kernel
void kernel_vmspace_create(vmspace_t *vmspace, Range total_memory_range);
// Create virtual memory space for user processes
vmspace_t *vmspace_create();

vmspace_t *vmspace_clone(vmspace_t *other);

void vmspace_switch(vmspace_t *vmspace);
void vmspace_destroy(vmspace_t *vmspace);
