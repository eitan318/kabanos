#pragma once
#include <stdbool.h>
#include <stdint.h>

// Internal constants
#define PAGE_SIZE 4096
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024

// Internal PTE flags
#define PTE_PRESENT (1 << 0)
#define PTE_WRITE (1 << 1)
#define PTE_USER (1 << 2)
#define PTE_WRITETHROUGH (1 << 3)
#define PTE_CACHEDISABLE (1 << 4)
#define PTE_ACCESSED (1 << 5)
#define PTE_DIRTY (1 << 6)
#define PTE_PAT (1 << 7)
#define PTE_GLOBAL (1 << 8)

// Internal PDE flags
#define PDE_PRESENT PTE_PRESENT
#define PDE_WRITE PTE_WRITE
#define PDE_USER PTE_USER
#define PDE_WRITETHROUGH PTE_WRITETHROUGH
#define PDE_CACHEDISABLE PTE_CACHEDISABLE
#define PDE_ACCESSED PTE_ACCESSED
#define PDE_SIZE (1 << 7)
#define PDE_GLOBAL PTE_GLOBAL

// Address manipulation
#define PAGE_FRAME_MASK 0xFFFFF000
#define PAGE_FLAGS_MASK 0x00000FFF
#define PAGE_DIRECTORY_INDEX(virt) (((uint32_t)(virt) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(virt) (((uint32_t)(virt) >> 12) & 0x3FF)
#define PAGE_OFFSET(virt) ((uint32_t)(virt)&0xFFF)

// Opaque type - hide the internal structure
typedef struct PageDirectory PageDirectory;

// Simplified flags that users actually care about
#define PAGE_WRITABLE (1 << 0) // Otherwise read-only
#define PAGE_USER (1 << 1)     // Otherwise kernel-only
#define PAGE_NOCACHE (1 << 2)  // Disable caching (for MMIO)

// Core API
PageDirectory *paging_create(void);
void paging_destroy(PageDirectory *page_dir);

bool paging_map(PageDirectory *page_dir, uint32_t virtual_addr,
                uint32_t physical_addr, uint32_t flags);
bool paging_unmap(PageDirectory *page_dir, uint32_t virtual_addr);
uint32_t paging_get_physical(PageDirectory *page_dir, uint32_t virtual_addr);

void paging_enable(PageDirectory *page_dir);
void paging_disable(void);
bool paging_is_enabled(void);

// Helper for mapping ranges (optional convenience function)
bool paging_map_range(PageDirectory *page_dir, uint32_t virtual_start,
                      uint32_t physical_start, uint32_t size, uint32_t flags);
