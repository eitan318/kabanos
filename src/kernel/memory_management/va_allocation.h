#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/types.h"
#include "hal.h"

bool va_alloc_region(arch_vm_t *vm, uint32_t virt_start, size_t size,
                     uint32_t flags);
void va_free_region(arch_vm_t *vm, uint32_t virt_start, size_t size);
