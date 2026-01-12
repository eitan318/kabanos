#include "boot_paging.h"
#include "memory_management/memdefs.h"
#include "memory_management/vmm.h"

// MUST be in .entry to access before paging
__attribute__((aligned(PAGE_SIZE), section(".entry.data"))) static uint32_t
    bootstrap_page_directory[PD_ENTRIES];

__attribute__((
    aligned(PAGE_SIZE),
    section(".entry.data"))) static uint32_t bootstrap_page_table[PD_ENTRIES];

static __attribute__((section(".entry"))) void bootstrap_fill_pt() {
  for (int i = 0; i < PD_ENTRIES; i++) {
    bootstrap_page_table[i] = (i << 12) | PAGE_PRESENT | PAGE_READWRITE;
  }
}

paddr_t __attribute__((section(".entry"))) bootstrap_create_pd() {
  bootstrap_fill_pt();

  // Clear PDEs
  for (int i = 0; i < PD_ENTRIES; i++) {
    bootstrap_page_directory[i] = 0;
  }

  // Identity map: 0x00000000 → 0x00000000
  bootstrap_page_directory[0] =
      ((uint32_t)bootstrap_page_table) | PAGE_PRESENT | PAGE_READWRITE;

  // Higher-half: 0xC0000000 → 0x00000000 (same physical memory)
  bootstrap_page_directory[KERNEL_BASE >> 22] =
      ((uint32_t)bootstrap_page_table) | PAGE_PRESENT | PAGE_READWRITE;

  return (uint32_t)bootstrap_page_directory;
}
