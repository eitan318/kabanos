#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hal.h"

bool va_alloc_region(page_dir_t *pd, uint32_t virt_start, size_t size,
                     uint32_t flags);
void va_free_region(page_dir_t *pd, uint32_t virt_start, size_t size);
