#include "paging.h"
#include "include/memory.h"

#define PHYS_TO_VIRT(addr) ((void *)(addr))
#define VIRT_TO_PHYS(addr) ((uint32_t)(addr))

/* Allocate and initialize a new page directory.
 * Returns a virtual pointer to the page directory, and writes its
 * physical address to out_phys_addr.
 */
PageDirectory *page_directory_create(FrameAllocator *allocator,
                                     uint32_t *out_phys_addr) {
  uint32_t phys_addr = frame_alloc(allocator);
  if (phys_addr == 0)
    return NULL;

  PageDirectory *pd = (PageDirectory *)PHYS_TO_VIRT(phys_addr);
  memset(pd, 0, PAGE_SIZE);

  if (out_phys_addr)
    *out_phys_addr = phys_addr;

  return pd;
}

/* Allocate and initialize a new page table.
 * Returns a virtual pointer to the page table, and writes its
 * physical address to out_phys_addr.
 */
PageTable *page_table_create(FrameAllocator *allocator,
                             uint32_t *out_phys_addr) {
  uint32_t phys_addr = frame_alloc(allocator);
  if (phys_addr == 0)
    return NULL;

  PageTable *pt = (PageTable *)PHYS_TO_VIRT(phys_addr);
  memset(pt, 0, PAGE_SIZE);

  if (out_phys_addr)
    *out_phys_addr = phys_addr;

  return pt;
}

/* Initialize paging system */
void paging_init(FrameAllocator *frame_allocator) {
  uint32_t kernel_dir_phys_addr = 0;
  PageDirectory *kernel_directory =
      page_directory_create(frame_allocator, &kernel_dir_phys_addr);

  if (!kernel_directory) {
    return; // Handle OOM
  }

  // TODO: Identity-map necessary regions when map_page is implemented

  paging_load_directory(kernel_dir_phys_addr);
  paging_enable();
}
