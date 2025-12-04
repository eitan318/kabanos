#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void* memset(void* ptr, int value, size_t num);

// Heap allocation functions
void kmalloc_init(void);  // Initialize heap
void* kmalloc(size_t size);
void kfree(void* ptr);

// Debug functions
size_t kmalloc_used(void);
size_t kmalloc_available(void);