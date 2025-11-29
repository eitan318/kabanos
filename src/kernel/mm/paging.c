#include "paging.h"
#include "../frame_allocator/frame_allocator.h"
#include "../include/memory.h"
#include "../include/stdio.h"
#include <stddef.h>

// Global frame allocator pointer
static FrameAllocator* g_frame_allocator = NULL;

/**
 * Initialize paging system with frame allocator
 */
void paging_init(FrameAllocator* allocator) {
    if (allocator == NULL) {
        printf("ERROR: Cannot initialize paging with NULL allocator\n");
        return;
    }
    g_frame_allocator = allocator;
    printf("Paging initialized successfully\n");
}

/**
 * Convert virtual address to physical address
 */
static uint32_t virtual_to_physical(void* virtual_addr) {
    return (uint32_t)virtual_addr;
}

/**
 * Allocate a physical frame and return its address
 */
static uint32_t allocate_frame(void) {
    if (g_frame_allocator == NULL) {
        printf("ERROR: Frame allocator not initialized\n");
        return 0;
    }
    
    uint64_t frame = frame_alloc(g_frame_allocator);
    if (frame == 0) {
        printf("ERROR: frame_alloc returned 0\n");
        return 0;
    }
    return (uint32_t)frame;
}

/**
 * Free a physical frame
 */
static void free_frame(uint32_t physical_addr) {
    if (g_frame_allocator == NULL) {
        printf("ERROR: Frame allocator not initialized in free_frame\n");
        return;
    }
    
    frame_free(g_frame_allocator, (uint64_t)physical_addr);
}

PageDirectoryT* page_directory_create(void) {
    printf("[page_directory_create] Starting...\n");
    
    uint32_t pd_physical = allocate_frame();
    if (pd_physical == 0) {
        printf("[page_directory_create] Failed to allocate frame\n");
        return NULL;
    }
    
    printf("[page_directory_create] Got frame at 0x%x\n", pd_physical);
    
    PageDirectoryT* page_dir = (PageDirectoryT*)pd_physical;
    
    printf("[page_directory_create] About to memset...\n");
    memset(page_dir, 0, sizeof(PageDirectoryT));
    printf("[page_directory_create] memset complete\n");
    
    printf("Page directory created at physical: 0x%x\n", pd_physical);
    
    return page_dir;
}

void page_directory_destroy(PageDirectoryT* page_dir) {
    if (page_dir == NULL) {
        return;
    }
    
    printf("[page_directory_destroy] Starting...\n");
    
    // Don't iterate - just free the directory itself for now
    uint32_t pd_physical = virtual_to_physical(page_dir);
    free_frame(pd_physical);
    
    printf("Page directory destroyed\n");
}

uint32_t page_directory_physical_get(PageDirectoryT* page_dir) {
    return virtual_to_physical(page_dir);
}

void page_directory_entry_set(PageDirectoryT* page_dir, uint32_t index, 
                               uint32_t page_table_physical, uint32_t flags) {
    if (page_dir == NULL || index >= PAGE_DIRECTORY_ENTRIES) {
        return;
    }
    
    PageDirectoryEntryT* pde = &page_dir->entries[index];
    
    pde->frame = (page_table_physical >> 12) & 0xFFFFF;
    pde->present = (flags & PDE_PRESENT) ? 1 : 0;
    pde->write = (flags & PDE_WRITE) ? 1 : 0;
    pde->user = (flags & PDE_USER) ? 1 : 0;
    pde->writethrough = (flags & PDE_WRITETHROUGH) ? 1 : 0;
    pde->cachedisable = (flags & PDE_CACHEDISABLE) ? 1 : 0;
    pde->size = (flags & PDE_SIZE) ? 1 : 0;
    pde->global = (flags & PDE_GLOBAL) ? 1 : 0;
    
    pde->zero = 0;
    pde->accessed = 0;
    pde->available = 0;
}

void page_table_entry_set(PageTableT* page_table, uint32_t index,
                          uint32_t physical_address, uint32_t flags) {
    if (page_table == NULL || index >= PAGE_TABLE_ENTRIES) {
        return;
    }
    
    PageTableEntryT* pte = &page_table->entries[index];
    
    pte->frame = (physical_address >> 12) & 0xFFFFF;
    pte->present = (flags & PTE_PRESENT) ? 1 : 0;
    pte->write = (flags & PTE_WRITE) ? 1 : 0;
    pte->user = (flags & PTE_USER) ? 1 : 0;
    pte->writethrough = (flags & PTE_WRITETHROUGH) ? 1 : 0;
    pte->cachedisable = (flags & PTE_CACHEDISABLE) ? 1 : 0;
    pte->pat = (flags & PTE_PAT) ? 1 : 0;
    pte->global = (flags & PTE_GLOBAL) ? 1 : 0;
    
    pte->dirty = 0;
    pte->accessed = 0;
    pte->available = 0;
}