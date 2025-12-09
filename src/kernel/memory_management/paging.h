#pragma once
#include <stdbool.h>
#include <stdint.h>

// Internal constants
#define PAGE_SIZE 4096
// Opaque type - hide the internal structure
typedef struct PageDirectory PageDirectory;

// Simplified flags that users actually care about
#define PAGE_WRITABLE (1 << 0) // Otherwise read-only
#define PAGE_USER (1 << 1)     // Otherwise kernel-only
#define PAGE_NOCACHE (1 << 2)  // Disable caching (for MMIO)

// Core API
PageDirectory *paging_create(void);
void paging_destroy(PageDirectory *page_dir);
void page_dir_load(uint32_t page_dir_phys_addr);

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

uint32_t paging_virt_addr_align_down(uint32_t addr);
uint32_t paging_virt_addr_align_up(uint32_t addr);
