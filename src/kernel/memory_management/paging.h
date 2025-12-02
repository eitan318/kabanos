#pragma once

#include "memory_management/frame_allocator.h"
#include <stdbool.h>
#include <stdint.h>

// Page size (4KB)
#define PAGE_SIZE 4096

// Number of entries in page directory/table
#define PAGE_DIRECTORY_ENTRIES 1024
#define PAGE_TABLE_ENTRIES 1024

// Page table entry flags
#define PTE_PRESENT (1 << 0)      // Page is present in memory
#define PTE_WRITE (1 << 1)        // Page is writable
#define PTE_USER (1 << 2)         // Page is user-accessible
#define PTE_WRITETHROUGH (1 << 3) // Write-through caching
#define PTE_CACHEDISABLE (1 << 4) // Cache disabled
#define PTE_ACCESSED (1 << 5)     // Page has been accessed
#define PTE_DIRTY (1 << 6)        // Page has been written to
#define PTE_PAT (1 << 7)          // Page Attribute Table
#define PTE_GLOBAL (1 << 8)       // Global page (not flushed from TLB)

// Page directory entry flags (same as PTE for most)
#define PDE_PRESENT PTE_PRESENT
#define PDE_WRITE PTE_WRITE
#define PDE_USER PTE_USER
#define PDE_WRITETHROUGH PTE_WRITETHROUGH
#define PDE_CACHEDISABLE PTE_CACHEDISABLE
#define PDE_ACCESSED PTE_ACCESSED
#define PDE_SIZE (1 << 7) // 0 = 4KB pages, 1 = 4MB pages
#define PDE_GLOBAL PTE_GLOBAL

// Mask to extract physical address from entry
#define PAGE_FRAME_MASK 0xFFFFF000
#define PAGE_FLAGS_MASK 0x00000FFF

// Address manipulation macros
#define PAGE_DIRECTORY_INDEX(virt) (((uint32_t)(virt) >> 22) & 0x3FF)
#define PAGE_TABLE_INDEX(virt) (((uint32_t)(virt) >> 12) & 0x3FF)
#define PAGE_OFFSET(virt) ((uint32_t)(virt)&0xFFF)

// Page Table Entry structure
typedef struct {
  uint32_t present : 1;      // Page present in memory
  uint32_t write : 1;        // Read/write permission
  uint32_t user : 1;         // User/supervisor
  uint32_t writethrough : 1; // Write-through caching
  uint32_t cachedisable : 1; // Cache disabled
  uint32_t accessed : 1;     // Has been accessed
  uint32_t dirty : 1;        // Has been written to
  uint32_t pat : 1;          // Page Attribute Table
  uint32_t global : 1;       // Global page
  uint32_t available : 3;    // Available for OS use
  uint32_t frame : 20;       // Physical frame address (bits 31-12)
} __attribute__((packed)) PageTableEntry;

// Page Directory Entry structure
typedef struct {
  uint32_t present : 1;      // Page table present
  uint32_t write : 1;        // Read/write permission
  uint32_t user : 1;         // User/supervisor
  uint32_t writethrough : 1; // Write-through caching
  uint32_t cachedisable : 1; // Cache disabled
  uint32_t accessed : 1;     // Has been accessed
  uint32_t zero : 1;         // Reserved (must be 0)
  uint32_t size : 1;         // Page size (0 = 4KB, 1 = 4MB)
  uint32_t global : 1;       // Global page
  uint32_t available : 3;    // Available for OS use
  uint32_t frame : 20;       // Page table physical address (bits 31-12)
} __attribute__((packed)) PageDirectoryEntry;

// Page Table (1024 entries)
typedef struct {
  PageTableEntry entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) PageTable;

// Page Directory (1024 entries)
typedef struct {
  PageDirectoryEntry entries[PAGE_DIRECTORY_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) PageDirectory;

/**
 * Create a new page directory
 * Allocates and initializes a page directory with all entries cleared
 *
 * @return Pointer to the new page directory, or NULL on failure
 */
PageDirectory *page_dir_create(void);

/**
 * Destroy a page directory
 * Frees all associated page tables and the directory itself
 *
 * @param page_dir Pointer to the page directory to destroy
 */
void page_dir_destroy(PageDirectory *page_dir);

/**
 * Get physical address of page directory (for loading into CR3)
 *
 * @param page_dir Pointer to the page directory
 * @return Physical address of the page directory
 */
uint32_t page_dir_physical_get(PageDirectory *page_dir);

/**
 * Helper function to set a page directory entry
 *
 * @param page_dir Pointer to the page directory
 * @param index Index of the entry to set
 * @param page_table_physical Physical address of the page table
 * @param flags Flags for the entry (PDE_PRESENT, PDE_WRITE, PDE_USER, etc.)
 */
void page_dir_entry_set(PageDirectory *page_dir, uint32_t index,
                        uint32_t page_table_physical, uint32_t flags);

/**
 * Helper function to set a page table entry
 *
 * @param page_table Pointer to the page table
 * @param index Index of the entry to set
 * @param physical_address Physical address to map
 * @param flags Flags for the entry (PTE_PRESENT, PTE_WRITE, PTE_USER, etc.)
 */
void page_table_entry_set(PageTable *page_table, uint32_t index,
                          uint32_t physical_address, uint32_t flags);

/**
 * Create a new page table
 *
 * @return Pointer to the new page table, or NULL on failure
 */
PageTable *page_table_create(void);

/**
 * Destroy a page table
 *
 * @param page_table Pointer to the page table to destroy
 */
void page_table_destroy(PageTable *page_table);

/**
 * Map a virtual address to a physical address
 *
 * @param page_dir Pointer to the page directory
 * @param virtual_addr Virtual address to map
 * @param physical_addr Physical address to map to
 * @param flags Flags for the mapping (PTE_PRESENT, PTE_WRITE, PTE_USER, etc.)
 * @return true on success, false on failure
 */
bool paging_page_map(PageDirectory *page_dir, uint32_t virtual_addr,
                     uint32_t physical_addr, uint32_t flags);

/**
 * Unmap a virtual address
 *
 * @param page_dir Pointer to the page directory
 * @param virtual_addr Virtual address to unmap
 * @return true on success, false on failure
 */
bool paging_page_unmap(PageDirectory *page_dir, uint32_t virtual_addr);

/**
 * Get physical address for a virtual address
 *
 * @param page_dir Pointer to the page directory
 * @param virtual_addr Virtual address to translate
 * @return Physical address, or 0 if not mapped
 */
uint32_t paging_physical_address_get(PageDirectory *page_dir,
                                     uint32_t virtual_addr);

/**
 * Enable paging by loading page directory into CR3
 *
 * @param page_dir Pointer to the page directory to enable
 */
void paging_enable(PageDirectory *page_dir);

/**
 * Disable paging
 */
void paging_disable(void);

/**
 * Check if paging is currently enabled
 *
 * @return true if paging is enabled, false otherwise
 */
bool paging_is_enabled(void);