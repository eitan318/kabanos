#pragma once
#include "vmm.h"
#include <stdint.h>

// Create initial virtual memory space for kernel
void kernel_vmspace_create(vmspace_t *vmspace);
// Create virtual memory space for user processes
vmspace_t *user_vmspace_creat();

void vmspace_switch(vmspace_t *vmspace);
void vmspace_destroy(vmspace_t *vmspace);
