#pragma once
#include <stdbool.h>
#include <stdint.h>

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
