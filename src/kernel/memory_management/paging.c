#include "paging.h"
#include "../include/stdio.h"
#include "memory_management/frame_allocator.h"
#include <stddef.h>

static bool g_paging_enabled = false;

/**
 * Convert virtual address to physical address (with paging enabled)
 */
static uint32_t virtual_to_physical_with_paging(void *virtual_addr) {
  uint32_t virt = (uint32_t)virtual_addr;
  // Get current page directory from CR3
  uint32_t cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));

  // CRITICAL: After paging is enabled, we must access the page directory
  // through its virtual address (identity-mapped), not its physical address
  PageDirectory *page_dir = (PageDirectory *)(cr3 & PAGE_FRAME_MASK);

  // Get page directory entry
  uint32_t pd_index = PAGE_DIRECTORY_INDEX(virt);
  PageDirectoryEntry *pde = &page_dir->entries[pd_index];

  if (!pde->present) {
    debugf("ERROR: Page table not present for virtual address 0x%x\n", virt);
    return 0;
  }

  // CRITICAL: The frame field contains the physical page number (bits 31-12)
  // With identity mapping, physical address == virtual address
  // So we can use the physical address directly as a virtual address
  uint32_t pt_physical = pde->frame << 12;
  PageTable *page_table = (PageTable *)pt_physical;

  // Get page table entry
  uint32_t pt_index = PAGE_TABLE_INDEX(virt);
  PageTableEntry *pte = &page_table->entries[pt_index];

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
static uint32_t virtual_to_physical(void *virtual_addr) {
  if (!g_paging_enabled) {
    return (uint32_t)virtual_addr;
  } else {
    return virtual_to_physical_with_paging(virtual_addr);
  }
}

/**
 * Allocate a physical frame and return its address
 */
static uint32_t frame_allocate(void) {
  uint64_t frame = frame_alloc();
  if (frame == 0) {
    debugf("ERROR: frame_alloc returned 0\n");
    return 0;
  }

  uint32_t frame32 = (uint32_t)frame;

  // Verify frame is page-aligned
  if (frame32 & 0xFFF) {
    debugf("ERROR: frame_alloc returned unaligned address 0x%x\n", frame32);
    return 0;
  }

  return frame32;
}

/**
 * Invalidate TLB entry for a virtual address
 */
static inline void invlpg(void *addr) {
  asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

/**
 * Safely clear a page of memory
 * This is critical because frames from the free list contain linked list
 * pointers!
 */
static void clear_page(void *addr) {
  // Clear using 32-bit writes for efficiency
  volatile uint32_t *ptr = (volatile uint32_t *)addr;

  // A page is 4096 bytes = 1024 dwords
  for (uint32_t i = 0; i < (PAGE_SIZE / 4); i++) {
    ptr[i] = 0;
  }
}

PageDirectory *page_dir_create(void) {
  uint32_t pd_physical = frame_allocate();
  if (pd_physical == 0) {
    debugf("ERROR: Failed to allocate frame for page directory\n");
    return NULL;
  }

  debugf("Page directory created at physical: 0x%x\n", pd_physical);

  PageDirectory *page_dir = (PageDirectory *)pd_physical;

  // CRITICAL: Clear the frame AFTER getting the pointer
  // The frame might contain free list pointers, so we must clear it
  clear_page(page_dir);

  return page_dir;
}

void page_dir_destroy(PageDirectory *page_dir) {
  if (page_dir == NULL) {
    return;
  }

  // Free all page tables that are present
  for (uint32_t i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
    if (page_dir->entries[i].present) {
      uint32_t pt_physical = page_dir->entries[i].frame << 12;
      frame_free(pt_physical);
    }
  }

  // Free the directory itself
  uint32_t pd_physical = virtual_to_physical(page_dir);
  frame_free(pd_physical);
}

uint32_t page_dir_physical_get(PageDirectory *page_dir) {
  return virtual_to_physical(page_dir);
}

void page_dir_entry_set(PageDirectory *page_dir, uint32_t index,
                        uint32_t page_table_physical, uint32_t flags) {
  if (page_dir == NULL || index >= PAGE_DIRECTORY_ENTRIES) {
    return;
  }

  PageDirectoryEntry *pde = &page_dir->entries[index];

  // Build entry as 32-bit value
  uint32_t entry = 0;

  // Set physical address (bits 31-12)
  entry |= (page_table_physical & PAGE_FRAME_MASK);

  // Set flags
  if (!!(flags & PDE_PRESENT))
    entry |= (1 << 0);
  if (!!(flags & PDE_WRITE))
    entry |= (1 << 1);
  if (!!(flags & PDE_USER))
    entry |= (1 << 2);
  if (!!(flags & PDE_WRITETHROUGH))
    entry |= (1 << 3);
  if (!!(flags & PDE_CACHEDISABLE))
    entry |= (1 << 4);
  if (!!(flags & PDE_SIZE))
    entry |= (1 << 7);
  if (!!(flags & PDE_GLOBAL))
    entry |= (1 << 8);

  // Write entry using volatile pointer to prevent optimization
  volatile uint32_t *entry_ptr = (volatile uint32_t *)pde;
  *entry_ptr = entry;
}

void page_table_entry_set(PageTable *page_table, uint32_t index,
                          uint32_t physical_address, uint32_t flags) {
  if (page_table == NULL || index >= PAGE_TABLE_ENTRIES) {
    return;
  }

  PageTableEntry *pte = &page_table->entries[index];

  // Build entry as 32-bit value
  uint32_t entry = 0;

  // Set physical address (bits 31-12)
  entry |= (physical_address & PAGE_FRAME_MASK);

  // Set flags
  if (!!(flags & PTE_PRESENT))
    entry |= (1 << 0);
  if (!!(flags & PTE_WRITE))
    entry |= (1 << 1);
  if (!!(flags & PTE_USER))
    entry |= (1 << 2);
  if (!!(flags & PTE_WRITETHROUGH))
    entry |= (1 << 3);
  if (!!(flags & PTE_CACHEDISABLE))
    entry |= (1 << 4);
  if (!!(flags & PTE_PAT))
    entry |= (1 << 7);
  if (!!(flags & PTE_GLOBAL))
    entry |= (1 << 8);

  // Write entry using volatile pointer to prevent optimization
  volatile uint32_t *entry_ptr = (volatile uint32_t *)pte;
  *entry_ptr = entry;
}

PageTable *page_table_create(void) {
  uint32_t pt_physical = frame_allocate();
  if (pt_physical == 0) {
    debugf("ERROR: Failed to allocate frame for page table\n");
    return NULL;
  }

  PageTable *page_table = (PageTable *)pt_physical;

  // CRITICAL: Clear the frame AFTER getting the pointer
  // The frame might contain free list pointers, so we must clear it
  clear_page(page_table);

  return page_table;
}

void page_table_destroy(PageTable *page_table) {
  if (page_table == NULL) {
    return;
  }

  uint32_t pt_physical = virtual_to_physical(page_table);
  frame_free(pt_physical);
}

/**
 * Get or create a page table for a given virtual address
 */
static PageTable *page_table_get_or_create(PageDirectory *page_dir,
                                           uint32_t virtual_addr) {
  if (page_dir == NULL) {
    return NULL;
  }

  uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
  PageDirectoryEntry *pde = &page_dir->entries[pd_index];

  // If page table already exists, return it
  if (pde->present) {
    uint32_t pt_physical = pde->frame << 12;
    return (PageTable *)pt_physical;
  }

  // Create new page table
  PageTable *page_table = page_table_create();
  if (page_table == NULL) {
    debugf("ERROR: Failed to create page table\n");
    return NULL;
  }

  // Get the physical address
  uint32_t pt_physical = (uint32_t)page_table;

  // Set the page directory entry
  page_dir_entry_set(page_dir, pd_index, pt_physical,
                     PDE_PRESENT | PDE_WRITE | PDE_USER);

  return page_table;
}

bool paging_page_map(PageDirectory *page_dir, uint32_t virtual_addr,
                     uint32_t physical_addr, uint32_t flags) {
  if (page_dir == NULL) {
    debugf("ERROR: NULL page directory\n");
    return false;
  }

  // Align addresses to page boundaries
  virtual_addr &= PAGE_FRAME_MASK;
  physical_addr &= PAGE_FRAME_MASK;

  // Get or create the page table for this virtual address
  PageTable *page_table = page_table_get_or_create(page_dir, virtual_addr);
  if (page_table == NULL) {
    debugf("ERROR: Failed to get/create page table for virt 0x%x\n",
           virtual_addr);
    return false;
  }

  // Set the page table entry
  uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
  page_table_entry_set(page_table, pt_index, physical_addr, flags);

  // Invalidate TLB entry (only if paging is enabled)
  if (g_paging_enabled) {
    invlpg((void *)virtual_addr);
  }

  return true;
}

bool paging_page_unmap(PageDirectory *page_dir, uint32_t virtual_addr) {
  if (page_dir == NULL) {
    return false;
  }

  virtual_addr &= PAGE_FRAME_MASK;

  uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
  PageDirectoryEntry *pde = &page_dir->entries[pd_index];

  if (!pde->present) {
    return false;
  }

  uint32_t pt_physical = pde->frame << 12;
  PageTable *page_table = (PageTable *)pt_physical;

  uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
  PageTableEntry *pte = &page_table->entries[pt_index];

  if (!pte->present) {
    return false;
  }

  // Clear the entry using volatile pointer
  volatile uint32_t *entry_ptr = (volatile uint32_t *)pte;
  *entry_ptr = 0;

  // Invalidate TLB entry (only if paging is enabled)
  if (g_paging_enabled) {
    invlpg((void *)virtual_addr);
  }

  return true;
}

uint32_t paging_physical_address_get(PageDirectory *page_dir,
                                     uint32_t virtual_addr) {
  if (page_dir == NULL) {
    return 0;
  }

  uint32_t pd_index = PAGE_DIRECTORY_INDEX(virtual_addr);
  PageDirectoryEntry *pde = &page_dir->entries[pd_index];

  if (!pde->present) {
    return 0;
  }

  uint32_t pt_physical = pde->frame << 12;
  PageTable *page_table = (PageTable *)pt_physical;

  uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
  PageTableEntry *pte = &page_table->entries[pt_index];

  if (!pte->present) {
    return 0;
  }

  // Combine frame address with page offset
  uint32_t page_physical = pte->frame << 12;
  uint32_t offset = PAGE_OFFSET(virtual_addr);

  return page_physical | offset;
}

void paging_enable(PageDirectory *page_dir) {
  if (page_dir == NULL) {
    debugf("ERROR: NULL page directory\n");
    return;
  }

  uint32_t pd_physical = page_dir_physical_get(page_dir);

  // Load CR3 with page directory address
  asm volatile("mov %0, %%cr3" ::"r"(pd_physical));

  // Enable paging by setting bit 31 of CR0
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000;
  asm volatile("mov %0, %%cr0" ::"r"(cr0));

  g_paging_enabled = true;
}

void paging_disable(void) {
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  cr0 &= ~0x80000000;
  asm volatile("mov %0, %%cr0" ::"r"(cr0));

  g_paging_enabled = false;
}

bool paging_is_enabled(void) { return g_paging_enabled; }
