#pragma once
#include "utils/math.h"
#include "utils/range.h"
#include <stddef.h>

#define EARLY_IDENTITY_MAP_SIZE (16 * 1024 * 1024) // 16MB

void early_mem_init(void);
void *early_pmm_alloc(size_t size);
Range early_pmm_get_used_range();
