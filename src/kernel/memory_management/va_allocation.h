#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_management/vmm.h"

bool va_alloc_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                     uint32_t flags, bool map_down);
void va_free_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                    bool map_down);
