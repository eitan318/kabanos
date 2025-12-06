#include "process/proccess_memory_alloc.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "process/pcb.h"

void allocate_heap(Pcb *pcb, void *heap_start, uint64_t heap_size,
                   PageDirectory *page_dir) {
  uint32_t pages_needed = (heap_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t virt_addr = (uint32_t)heap_start;

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = frame_alloc();
    if (!paging_map(page_dir, virt_addr, phys, PAGE_USER | PAGE_WRITABLE)) {
      // handle mapping failure
    }
    virt_addr += PAGE_SIZE; // move up one page
  }

  pcb->heap_start = (uint32_t)heap_start;
  pcb->heap_end = (uint32_t)heap_start + heap_size;
  pcb->brk = (uint32_t)heap_start;
}

void allocate_stack(Pcb *pcb, uint32_t stack_top, uint64_t stack_size,
                    PageDirectory *page_dir) {
  uint32_t pages_needed = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t virt_addr = stack_top - PAGE_SIZE; // start at top - one page

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = frame_alloc();
    if (!paging_map(page_dir, virt_addr, phys, PAGE_USER | PAGE_WRITABLE)) {
      // handle allocation/map failure
    }
    virt_addr -= PAGE_SIZE; // move down one page
  }

  pcb->cpu_context.esp = stack_top; // stack pointer at top
  pcb->stack_top = stack_top - stack_size;
  pcb->stack_size = stack_size;
}
