#pragma once
#include "vmm.h"
#include <stdint.h>

// Create initial virtual memory space for kernel
void kernel_vmspace_creat(vmspace_t *vmspace);

// Create virtual memory space for user processes
vmspace_t *user_vmspace_creat();

void vmspace_destroy(vmspace_t *vmspace);
