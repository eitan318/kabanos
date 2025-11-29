#pragma once
#include "frame_allocator/frame_allocator.h"
#include "utils/binary.h"
#include <stddef.h>
#include <stdint.h>

/* Architecture constants */
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12 // 2^12 = 4096 bytes
#define PAGE_OFFSET_BITS PAGE_SHIFT

#define PT_INDEX_BITS 10 // page table = 1024 entries
#define PD_INDEX_BITS 10 // page directory = 1024 entries

#define PAGE_ENTRIES (1 << PT_INDEX_BITS) // 1024

/* Masks for extracting address and flags from entries */
#define PAGE_ADDR_MASK (~((1U << PAGE_OFFSET_BITS) - 1)) // 0xFFFFF000
#define PAGE_FLAGS_MASK ((1U << PAGE_OFFSET_BITS) - 1)   // 0x00000FFF

/* Page flags */
enum PAGE_FLAG_BITS {
  PAGE_PRESENT = (1 << 0),
  PAGE_RW = (1 << 1),
  PAGE_USER = (1 << 2),
  PAGE_WRITE_THROUGH = (1 << 3),
  PAGE_CACHE_DISABLE = (1 << 4),
  PAGE_ACCESSED = (1 << 5),
  PAGE_DIRTY = (1 << 6),
  PAGE_SIZE_4MB = (1 << 7),
};

/* Page entry type */
typedef uint32_t page_entry_t;

/* Page directory / table structures */
typedef struct {
  page_entry_t entries[PAGE_ENTRIES];
} PageDirectory;

typedef struct {
  page_entry_t entries[PAGE_ENTRIES];
} PageTable;

/* Page entry manipulation */
static inline uint32_t page_entry_get_addr(page_entry_t entry) {
  return entry & PAGE_ADDR_MASK;
}

static inline uint32_t page_entry_get_flags(page_entry_t entry) {
  return entry & PAGE_FLAGS_MASK;
}

static inline page_entry_t page_entry_create(uint32_t addr, uint32_t flags) {
  return (addr & PAGE_ADDR_MASK) | (flags & PAGE_FLAGS_MASK);
}

static inline int page_entry_is_present(page_entry_t entry) {
  return MASK_CHECK(entry, PAGE_PRESENT);
}

/* Virtual address decomposition */
static inline uint32_t virt_to_pd_index(uint32_t virt_addr) {
  return GET_BITS(virt_addr,
                  PAGE_OFFSET_BITS + PT_INDEX_BITS, // 12 + 10 = 22
                  PD_INDEX_BITS                     // 10 bits long
  );
}

static inline uint32_t virt_to_pt_index(uint32_t virt_addr) {
  return GET_BITS(virt_addr,
                  PAGE_OFFSET_BITS, // 12
                  PT_INDEX_BITS     // 10 bits long
  );
}

/* CPU paging control */
static inline void paging_enable(void) {
  uint32_t cr0;
  asm volatile("mov %%cr0, %0" : "=r"(cr0));
  MASK_SET(cr0, 1 << 31);
  asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

static inline void paging_load_directory(uint32_t phys_addr) {
  asm volatile("mov %0, %%cr3" ::"r"(phys_addr));
}

static inline void paging_flush_tlb_entry(uint32_t virt_addr) {
  asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

static inline void paging_flush_tlb_all(void) {
  uint32_t cr3;
  asm volatile("mov %%cr3, %0" : "=r"(cr3));
  asm volatile("mov %0, %%cr3" ::"r"(cr3));
}

/* Allocation functions */
PageDirectory *page_directory_create(FrameAllocator *allocator,
                                     uint32_t *out_phys_addr);

PageTable *page_table_create(FrameAllocator *allocator,
                             uint32_t *out_phys_addr);

/* Paging initialization */
void paging_init(FrameAllocator *frame_allocator);
