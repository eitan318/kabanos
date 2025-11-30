#include "paging.h"
#include "../frame_allocator/frame_allocator.h"
#include "../include/memory.h"
#include "../include/stdio.h"
#include <stddef.h>

// Global frame allocator pointer
static FrameAllocator* g_frame_allocator = NULL;

// Global flag to track paging state
static bool g_paging_enabled = false;

/**
 * Initialize paging system with frame allocator
 */
void paging_init(FrameAllocator* allocator) {
    if (allocator == NULL) {
        debugf("ERROR: Cannot initialize paging with NULL allocator\n");
        return;
    }
    g_frame_allocator = allocator;
    g_paging_enabled = false;
    debugf("Paging initialized successfully\n");
}

/**
 * Convert virtual address to physical address (with paging enabled)
 */
static uint32_t physical_with_paging_virtual_to(void* virtual_addr) {
    uint32_t virt = (uint32_t)virtual_addr;
    
    // Get current page directory from CR3
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    PageDirectoryT* page_dir = (PageDirectoryT*)(cr3 & PAGE_FRAME_MASK);
    
    // Get page directory entry
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virt);
    PageDirectoryEntryT* pde = &page_dir->entries[pd_index];
    
    if (!pde->present) {
        debugf("ERROR: Page table not present for virtual address 0x%x\n", virt);
        return 0;
    }
    
    // Get page table
    uint32_t pt_physical = pde->frame << 12;
    PageTableT* page_table = (PageTableT*)pt_physical;
    
    // Get page table entry
    uint32_t pt_index = PAGE_TABLE_INDEX(virt);
    PageTableEntryT* pte = &page_table->entries[pt_index];
    
    if (!pte->present) {
        debugf("ERROR: Page not present for virtual address 0x%x\n", virt);
        return 0;
    }
    
    // Return physical address
    uint32_t page_physical = pte->frame << 12;
    uint32_t offset = PAGE_OFFSET(virt);
    
    return page_physical | offset;
}

/**
 * Convert virtual address to physical address
 * Uses appropriate method based on whether paging is enabled
 */
static uint32_t physical_virtual_to(void* virtual_addr) {
    if (!g_paging_enabled) {
        // Before paging: identity mapping
        return (uint32_t)virtual_addr;
    } else {
        // After paging: walk page tables
        return physical_with_paging_virtual_to(virtual_addr);
    }
}

/**
 * Allocate a physical frame and return its address
 */
static uint32_t frame_allocate(void) {
    if (g_frame_allocator == NULL) {
        debugf("ERROR: Frame allocator not initialized\n");
        return 0;
    }
    
    uint64_t frame = frame_alloc(g_frame_allocator);
    if (frame == 0) {
        debugf("ERROR: frame_alloc returned 0\n");
        return 0;
    }
    return (uint32_t)frame;
}

/**
 * Free a physical frame
 */
static void frame_free_internal(uint32_t physical_addr) {
    if (g_frame_allocator == NULL) {
        debugf("ERROR: Frame allocator not initialized in frame_free_internal\n");
        return;
    }
    
    frame_free(g_frame_allocator, (uint64_t)physical_addr);
}

/**
 * Invalidate TLB entry for a virtual address
 */
static inline void invlpg(void* addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

/**
 * Create a new page directory
 */
PageDirectoryT* page_directory_create(void) {
    debugf("[page_directory_create] Starting...\n");
    
    uint32_t pd_physical = frame_allocate();
    if (pd_physical == 0) {
        debugf("[page_directory_create] Failed to allocate frame\n");
        return NULL;
    }
    
    debugf("[page_directory_create] Got frame at 0x%x\n", pd_physical);
    
    PageDirectoryT* page_dir = (PageDirectoryT*)pd_physical;
    
    debugf("[page_directory_create] About to memset...\n");
    memset(page_dir, 0, sizeof(PageDirectoryT));
    debugf("[page_directory_create] memset complete\n");
    
    debugf("Page directory created at physical: 0x%x\n", pd_physical);
    
    return page_dir;
}

/**
 * Destroy a page directory and free all associated page tables
 */
void page_directory_destroy(PageDirectoryT* page_dir) {
    if (page_dir == NULL) {
        return;
    }
    
    debugf("[page_directory_destroy] Starting...\n");
    
    // Free all page tables that are present
    for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        if (page_dir->entries[i].present) {
            uint32_t pt_physical = page_dir->entries[i].frame << 12;
            debugf("[page_directory_destroy] Freeing page table at index %u (0x%x)\n", 
                   i, pt_physical);
            frame_free_internal(pt_physical);
        }
    }
    
    // Free the directory itself
    uint32_t pd_physical = physical_virtual_to(page_dir);
    frame_free_internal(pd_physical);
    
    debugf("Page directory destroyed\n");
}

/**
 * Get physical address of page directory
 */
uint32_t page_directory_physical_get(PageDirectoryT* page_dir) {
    return physical_virtual_to(page_dir);
}

/**
 * Set a page directory entry
 */
void page_directory_entry_set(PageDirectoryT* page_dir, uint32_t index, 
                               uint32_t page_table_physical, uint32_t flags) {
    if (page_dir == NULL || index >= PAGE_DIRECTORY_ENTRIES) {
        return;
    }
    
    PageDirectoryEntryT* pde = &page_dir->entries[index];
    
    // Build entry as 32-bit value
    uint32_t entry = 0;
    
    // Set physical address (bits 31-12)
    entry |= (page_table_physical & PAGE_FRAME_MASK);
    
    // Set flags
    if (!!(flags & PDE_PRESENT))      entry |= (1 << 0);
    if (!!(flags & PDE_WRITE))        entry |= (1 << 1);
    if (!!(flags & PDE_USER))         entry |= (1 << 2);
    if (!!(flags & PDE_WRITETHROUGH)) entry |= (1 << 3);
    if (!!(flags & PDE_CACHEDISABLE)) entry |= (1 << 4);
    if (!!(flags & PDE_SIZE))         entry |= (1 << 7);
    if (!!(flags & PDE_GLOBAL))       entry |= (1 << 8);
    
    // Write entry using volatile pointer to prevent optimization
    volatile uint32_t* entry_ptr = (volatile uint32_t*)pde;
    *entry_ptr = entry;
}

/**
 * Set a page table entry
 */
void page_table_entry_set(PageTableT* page_table, uint32_t index,
                          uint32_t physical_address, uint32_t flags) {
    if (page_table == NULL || index >= PAGE_TABLE_ENTRIES) {
        return;
    }
    
    PageTableEntryT* pte = &page_table->entries[index];
    
    // Build entry as 32-bit value
    uint32_t entry = 0;
    
    // Set physical address (bits 31-12)
    entry |= (physical_address & PAGE_FRAME_MASK);
    
    // Set flags
    if (!!(flags & PTE_PRESENT))      entry |= (1 << 0);
    if (!!(flags & PTE_WRITE))        entry |= (1 << 1);
    if (!!(flags & PTE_USER))         entry |= (1 << 2);
    if (!!(flags & PTE_WRITETHROUGH)) entry |= (1 << 3);
    if (!!(flags & PTE_CACHEDISABLE)) entry |= (1 << 4);
    if (!!(flags & PTE_PAT))          entry |= (1 << 7);
    if (!!(flags & PTE_GLOBAL))       entry |= (1 << 8);
    
    // Write entry using volatile pointer to prevent optimization
    volatile uint32_t* entry_ptr = (volatile uint32_t*)pte;
    *entry_ptr = entry;
}

/**
 * Create a new page table
 */
PageTableT* page_table_create(void) {
    uint32_t pt_physical = frame_allocate();
    if (pt_physical == 0) {
        debugf("[page_table_create] Failed to allocate frame\n");
        return NULL;
    }
    
    PageTableT* page_table = (PageTableT*)pt_physical;
    memset(page_table, 0, sizeof(PageTableT));
    
    debugf("[page_table_create] Page table created at 0x%x\n", pt_physical);
    
    return page_table;
}

/**
 * Destroy a page table
 */
void page_table_destroy(PageTableT* page_table) {
    if (page_table == NULL) {
        return;
    }
    
    uint32_t pt_physical = physical_virtual_to(page_table);
    frame_free_internal(pt_physical);
    
    debugf("[page_table_destroy] Page table at 0x%x destroyed\n", pt_physical);
}

/**
 * Get or create a page table for a given virtual address
 */
static PageTableT* page_table_get_or_create(PageDirectoryT* page_dir, uint32_t virtual_addr) {
    if (page_dir == NULL) {
        return NULL;
    }
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    PageDirectoryEntryT* pde = &page_dir->entries[pd_index];
    
    // If page table already exists, return it
    if (pde->present) {
        uint32_t pt_physical = pde->frame << 12;
        return (PageTableT*)pt_physical;
    }
    
    // Create new page table
    PageTableT* page_table = page_table_create();
    if (page_table == NULL) {
        debugf("[page_table_get_or_create] Failed to create page table\n");
        return NULL;
    }
    
    // Set the page directory entry
    uint32_t pt_physical = physical_virtual_to(page_table);
    page_directory_entry_set(page_dir, pd_index, pt_physical, 
                             PDE_PRESENT | PDE_WRITE | PDE_USER);
    
    return page_table;
}

/**
 * Map a virtual page to a physical page
 */
bool paging_page_map(PageDirectoryT* page_dir, uint32_t virtual_addr, 
                     uint32_t physical_addr, uint32_t flags) {
    if (page_dir == NULL) {
        debugf("[paging_page_map] NULL page directory\n");
        return false;
    }
    
    // Align addresses to page boundaries
    virtual_addr &= PAGE_FRAME_MASK;
    physical_addr &= PAGE_FRAME_MASK;
    
    debugf("[paging_page_map] Mapping virt=0x%x to phys=0x%x with flags=0x%x\n",
           virtual_addr, physical_addr, flags);
    
    // Get or create the page table for this virtual address
    PageTableT* page_table = page_table_get_or_create(page_dir, virtual_addr);
    if (page_table == NULL) {
        debugf("[paging_page_map] Failed to get/create page table\n");
        return false;
    }
    
    // Set the page table entry
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    page_table_entry_set(page_table, pt_index, physical_addr, flags);
    
    // Invalidate TLB entry (only if paging is enabled)
    if (g_paging_enabled) {
        invlpg((void*)virtual_addr);
    }
    
    debugf("[paging_page_map] Successfully mapped page\n");
    return true;
}

/**
 * Unmap a virtual page
 */
bool paging_page_unmap(PageDirectoryT* page_dir, uint32_t virtual_addr) {
    if (page_dir == NULL) {
        return false;
    }
    
    virtual_addr &= PAGE_FRAME_MASK;
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    PageDirectoryEntryT* pde = &page_dir->entries[pd_index];
    
    if (!pde->present) {
        debugf("[paging_page_unmap] Page table not present\n");
        return false;
    }
    
    uint32_t pt_physical = pde->frame << 12;
    PageTableT* page_table = (PageTableT*)pt_physical;
    
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    PageTableEntryT* pte = &page_table->entries[pt_index];
    
    if (!pte->present) {
        debugf("[paging_page_unmap] Page not present\n");
        return false;
    }
    
    // Clear the entry using volatile pointer
    volatile uint32_t* entry_ptr = (volatile uint32_t*)pte;
    *entry_ptr = 0;
    
    // Invalidate TLB entry (only if paging is enabled)
    if (g_paging_enabled) {
        invlpg((void*)virtual_addr);
    }
    
    debugf("[paging_page_unmap] Successfully unmapped page at 0x%x\n", virtual_addr);
    return true;
}

/**
 * Get physical address for a virtual address
 */
uint32_t paging_physical_address_get(PageDirectoryT* page_dir, uint32_t virtual_addr) {
    if (page_dir == NULL) {
        return 0;
    }
    
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
    PageDirectoryEntryT* pde = &page_dir->entries[pd_index];
    
    if (!pde->present) {
        return 0;
    }
    
    uint32_t pt_physical = pde->frame << 12;
    PageTableT* page_table = (PageTableT*)pt_physical;
    
    uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
    PageTableEntryT* pte = &page_table->entries[pt_index];
    
    if (!pte->present) {
        return 0;
    }
    
    // Combine frame address with page offset
    uint32_t page_physical = pte->frame << 12;
    uint32_t offset = PAGE_OFFSET(virtual_addr);
    
    return page_physical | offset;
}

/**
 * Enable paging by loading page directory into CR3
 */
void paging_enable(PageDirectoryT* page_dir) {
    if (page_dir == NULL) {
        debugf("[paging_enable] NULL page directory\n");
        return;
    }
    
    uint32_t pd_physical = page_directory_physical_get(page_dir);
    
    debugf("[paging_enable] Loading CR3 with 0x%x\n", pd_physical);
    
    // Load CR3 with page directory address
    asm volatile("mov %0, %%cr3" :: "r"(pd_physical));
    
    // Enable paging by setting bit 31 of CR0
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    
    g_paging_enabled = true;
    
    debugf("Paging enabled with directory at 0x%x\n", pd_physical);
}

/**
 * Disable paging
 */
void paging_disable(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000;
    asm volatile("mov %0, %%cr0" :: "r"(cr0));
    
    g_paging_enabled = false;
    
    debugf("Paging disabled\n");
}

/**
 * Check if paging is currently enabled
 */
bool paging_is_enabled(void) {
    return g_paging_enabled;
}