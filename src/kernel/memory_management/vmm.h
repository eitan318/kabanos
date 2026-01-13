#pragma once
#include "pmm.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// FLAGS
#define PAGE_PRESENT 0x1
#define PAGE_READWRITE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

#define PD_ENTRIES PAGE_SIZE / sizeof(uint32_t)

typedef uint32_t page_dir_t;
typedef uint32_t vaddr_t;

// Virtual Memory Mapping
bool vm_map(page_dir_t *pd, vaddr_t va, paddr_t pa, uint32_t flags);
bool vm_unmap(page_dir_t *pd, vaddr_t va);

paddr_t vm_translate(page_dir_t *pd, vaddr_t va);

// Efficient range mapping/Unmapping
bool vm_map_range(page_dir_t *pd_virt, paddr_t pa_start, vaddr_t va_start,
                  size_t size, uint32_t flags);
bool vm_unmap_range(page_dir_t *pd_virt, vaddr_t va_start, size_t size);
