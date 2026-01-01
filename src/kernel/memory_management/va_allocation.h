#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory_management/paging.h"

void va_allocator_init(uint32_t start, uint32_t end, PageDirectory *page_dir);
void *va_alloc(size_t size, bool zero);
void va_free(void *ptr, size_t size); // optional stub for now
