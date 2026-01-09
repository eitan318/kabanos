#pragma once
#include "pmm.h"
#include <stdbool.h>
#include <stdint.h>

// FLAGS
#define PAGE_PRESENT 0x1
#define PAGE_READWRITE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

#define PD_ENTRIES PAGE_SIZE / sizeof(uint32_t)

typedef uint32_t page_dir_t;

typedef struct vmspace_t {
  uint32_t *pd;
  paddr_t pd_phys;
} vmspace_t;

typedef uint32_t vaddr_t;

// Virtual Memory Mapping
bool vm_map(uint32_t *pd, vaddr_t va, paddr_t pa, uint32_t flags);
bool vm_unmap(uint32_t *pd, vaddr_t va);
paddr_t vm_translate(uint32_t *pd, vaddr_t va);

// Virtual Memory Space
vmspace_t *user_vmspace_creat();
void kernel_vmspace_creat(vmspace_t *vmspace);
void vmspace_destroy(vmspace_t *vmspace);
void vmspace_switch(vmspace_t *vmspace);
