#pragma once
#include "utils/math.h"
#include "utils/range.h"
#include <stddef.h>

#define EARLY_PMM_SIZE PAGE_SIZE * 4

void early_mem_init(void);
void *early_pmm_vm_alloc(size_t size);
