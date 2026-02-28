#pragma once
#include "klib/stdbool.h"
#include "klib/stddef.h"
#include "klib/stdint.h"

#include "arch/types.h"

bool va_alloc_region(arch_vm_t *vm, uint32_t virt_start, size_t size,
                     uint32_t flags);
void va_free_region(arch_vm_t *vm, uint32_t virt_start, size_t size);
