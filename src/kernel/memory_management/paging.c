#include "paging.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include <stddef.h>

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

// Internal structures
typedef struct {
  uint32_t present : 1;
  uint32_t write : 1;
  uint32_t user : 1;
  uint32_t writethrough : 1;
  uint32_t cachedisable : 1;
  uint32_t accessed : 1;
  uint32_t dirty : 1;
  uint32_t pat : 1;
  uint32_t global : 1;
  uint32_t available : 3;
  uint32_t frame : 20;
} __attribute__((packed)) PageTableEntry;

typedef struct {
  uint32_t present : 1;
  uint32_t write : 1;
  uint32_t user : 1;
  uint32_t writethrough : 1;
  uint32_t cachedisable : 1;
  uint32_t accessed : 1;
  uint32_t zero : 1;
  uint32_t size : 1;
  uint32_t global : 1;
  uint32_t available : 3;
  uint32_t frame : 20;
} __attribute__((packed)) PageDirectoryEntry;

typedef struct {
  PageTableEntry entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) PageTable;

// PageDirectory is now defined here (not in header)
struct PageDirectory {
  PageDirectoryEntry entries[PAGE_DIRECTORY_ENTRIES];
} __attribute__((aligned(PAGE_SIZE)));

static bool g_paging_enabled = false;

/**
 * Convert simple user flags to internal PTE flags
 */
static uint32_t flags_to_pte(uint32_t simple_flags) {
  uint32_t pte_flags = PTE_PRESENT; // Always present when mapping

  if (simple_flags & PAGE_WRITABLE) {
    pte_flags |= PTE_WRITE;
  }

  if (simple_flags & PAGE_USER) {
    pte_flags |= PTE_USER;
  }

  if (simple_flags & PAGE_NOCACHE) {
    pte_flags |= PTE_CACHEDISABLE;
  }

  return pte_flags;
}

/**
 * Convert simple user flags to internal PDE flags
 */
static uint32_t flags_to_pde(uint32_t simple_flags) {
  uint32_t pde_flags = PDE_PRESENT; // Always present when mapping

  if (simple_flags & PAGE_WRITABLE) {
    pde_flags |= PDE_WRITE;
  }

  if (simple_flags & PAGE_USER) {
    pde_flags |= PDE_USER;
  }

  if (simple_flags & PAGE_NOCACHE) {
    pde_flags |= PDE_CACHEDISABLE;
  }

  return pde_flags;
}

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
 */
static uint32_t virtual_to_physical(void *virtual_addr) {
  if (!g_paging_enabled) {
    return (uint32_t)virtual_addr;
  } else {
    return virtual_to_physical_with_paging(virtual_addr);
  }
}

/**
 * Invalidate TLB entry for a virtual address
 */
static inline void invlpg(void *addr) {
  asm volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

/**
 * Safely clear a page of memory
 */
static void clear_page(void *addr) {
  volatile uint32_t *ptr = (volatile uint32_t *)addr;

  // A page is 4096 bytes = 1024 dwords
  for (uint32_t i = 0; i < (PAGE_SIZE / 4); i++) {
    ptr[i] = 0;
  }
}

/**
 * Create a page table
 */
static PageTable *page_table_create(void) {
  uint32_t pt_physical = frame_alloc();
  if (pt_physical == 0) {
    debugf("ERROR: Failed to allocate frame for page table\n");
    return NULL;
  }

  PageTable *page_table = (PageTable *)pt_physical;
  clear_page(page_table);

  return page_table;
}

/**
 * Destroy a page table
 */
static void page_table_destroy(PageTable *page_table) {
  if (page_table == NULL) {
    return;
  }

  uint32_t pt_physical = virtual_to_physical(page_table);
  frame_free(pt_physical);
}

/**
 * Set a page directory entry
 */
static void page_dir_entry_set(PageDirectory *page_dir, uint32_t index,
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

/**
 * Set a page table entry
 */
static void page_table_entry_set(PageTable *page_table, uint32_t index,
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

/**
 * Get or create a page table for a given virtual address
 */
static PageTable *page_table_get_or_create(PageDirectory *page_dir,
                                           uint32_t virtual_addr,
                                           uint32_t flags) {
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

  // Set the page directory entry with appropriate flags
  uint32_t pde_flags = flags_to_pde(flags);
  page_dir_entry_set(page_dir, pd_index, pt_physical, pde_flags);

  return page_table;
}

// ============================================================================
// Public API implementation
// ============================================================================

PageDirectory *paging_create(void) {
  uint32_t pd_physical = frame_alloc();
  if (pd_physical == 0) {
    debugf("ERROR: Failed to allocate frame for page directory\n");
    return NULL;
  }

  debugf("Page directory created at physical: 0x%x\n", pd_physical);

  PageDirectory *page_dir = (PageDirectory *)pd_physical;
  clear_page(page_dir);

  return page_dir;
}

void paging_destroy(PageDirectory *page_dir) {
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

bool paging_map(PageDirectory *page_dir, uint32_t virtual_addr,
                uint32_t physical_addr, uint32_t flags) {
  if (page_dir == NULL) {
    debugf("ERROR: NULL page directory\n");
    return false;
  }

  // Align addresses to page boundaries
  virtual_addr &= PAGE_FRAME_MASK;
  physical_addr &= PAGE_FRAME_MASK;

  // Get or create the page table for this virtual address
  PageTable *page_table =
      page_table_get_or_create(page_dir, virtual_addr, flags);
  if (page_table == NULL) {
    debugf("ERROR: Failed to get/create page table for virt 0x%x\n",
           virtual_addr);
    return false;
  }

  // Convert simple flags to internal PTE flags
  uint32_t pte_flags = flags_to_pte(flags);

  // Set the page table entry
  uint32_t pt_index = PAGE_TABLE_INDEX(virtual_addr);
  page_table_entry_set(page_table, pt_index, physical_addr, pte_flags);

  // Invalidate TLB entry (only if paging is enabled)
  if (g_paging_enabled) {
    invlpg((void *)virtual_addr);
  }

  return true;
}

bool paging_unmap(PageDirectory *page_dir, uint32_t virtual_addr) {
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

uint32_t paging_get_physical(PageDirectory *page_dir, uint32_t virtual_addr) {
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

bool paging_map_range(PageDirectory *page_dir, uint32_t virtual_start,
                      uint32_t physical_start, uint32_t size, uint32_t flags) {
  if (page_dir == NULL || size == 0) {
    return false;
  }

  // Align addresses and size to page boundaries
  virtual_start &= PAGE_FRAME_MASK;
  physical_start &= PAGE_FRAME_MASK;

  // Calculate number of pages (round up)
  uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

  // Map each page
  for (uint32_t i = 0; i < num_pages; i++) {
    uint32_t virt = virtual_start + (i * PAGE_SIZE);
    uint32_t phys = physical_start + (i * PAGE_SIZE);

    if (!paging_map(page_dir, virt, phys, flags)) {
      debugf("ERROR: Failed to map page %u in range\n", i);
      return false;
    }
  }

  return true;
}

void page_dir_load(uint32_t page_dir_phys_addr) {
  asm volatile("mov %0, %%cr3" ::"r"(page_dir_phys_addr));
}

void paging_enable(PageDirectory *page_dir) {
  if (page_dir == NULL) {
    debugf("ERROR: NULL page directory\n");
    return;
  }

  uint32_t pd_physical = virtual_to_physical(page_dir);

  // Load CR3 with page directory address
  page_dir_load(pd_physical);

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

uint32_t paging_virt_addr_align_down(uint32_t addr) {
  return addr & PAGE_FRAME_MASK; // private
}

uint32_t paging_virt_addr_align_up(uint32_t addr) {
  return (addr + PAGE_SIZE - 1) & PAGE_FRAME_MASK;
}

bool paging_is_enabled(void) { return g_paging_enabled; }
