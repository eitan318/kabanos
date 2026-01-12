#pragma once
#include "memdefs.h"
#include "memory_management/vmm.h"
#include <stddef.h>

void early_pmm_disable();
void early_pmm_init(uintptr_t start_phys_addr);
void *early_pmm_vm_alloc(size_t size);
