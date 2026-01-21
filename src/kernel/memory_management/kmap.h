#pragma once
#include <stdint.h>

// Map physical mem to temp kernel virt memory
void *kmap(uint32_t phys_addr);

// Unmap kernel virt memory
void kunmap(void);
