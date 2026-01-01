#include "task.h"
#include "arch/i686/gdt.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "memory_management/va_allocation.h"

extern PageDirectory *g_kernel_page_dir; // global

bool allocate_user_stack(PCB *pcb, uint32_t stack_top, uint32_t stack_size,
                         PageDirectory *page_dir) {
  if (!pcb || !page_dir || stack_size == 0) {
    return false;
  }

  uint32_t pages_needed = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
  uint32_t virt_addr = stack_top - PAGE_SIZE; // Start at top - one page
  uint32_t pages_allocated = 0;

  for (uint32_t i = 0; i < pages_needed; i++) {
    uint32_t phys = frame_alloc();
    if (phys == 0) {
      debugf("Frame allocation failed in allocate_stack at page %u\n", i);
      goto cleanup_on_error;
    }

    if (!paging_map(page_dir, virt_addr, phys, PAGE_USER | PAGE_WRITABLE)) {
      debugf("Mapping failure in allocate_stack at virt=0x%x\n", virt_addr);
      frame_free(phys); // Free the frame we just allocated
      goto cleanup_on_error;
    }

    pages_allocated++;
    virt_addr -= PAGE_SIZE;
  }

  pcb->user_esp = (uint32_t *)stack_top;

  return true;

cleanup_on_error:
  // Roll back partial allocation
  virt_addr = stack_top - PAGE_SIZE;
  for (uint32_t i = 0; i < pages_allocated; i++) {
    uint32_t phys = paging_get_physical(page_dir, virt_addr);
    if (phys != 0) {
      paging_unmap(page_dir, virt_addr);
      frame_free(phys);
    }
    virt_addr -= PAGE_SIZE;
  }
  return false;
}

#define KERNEL_STACK_SIZE PAGE_SIZE

// this shouild match isr.asm isr_common for preeamptive schedualing
void setup_pcb(PCB *p, void (*entry)(void)) {
  PageDirectory *page_dir = paging_create();
  p->cr3 = (uint32_t)page_dir;

  allocate_user_stack(p, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, page_dir);
  p->kernel_stack = va_alloc(KERNEL_STACK_SIZE, true);
  p->kernel_esp = (uint32_t *)(p->kernel_stack + KERNEL_STACK_SIZE);

  // Map same physical frame into the process page table
  uint32_t phys =
      paging_get_physical(g_kernel_page_dir, (uint32_t)p->kernel_stack);
  paging_map(page_dir, (uint32_t)p->kernel_stack, phys, PAGE_WRITABLE);

  // build iret frame on kernel stack, NOT user stack
  uint32_t *stk = p->kernel_esp;

  // ---- user mode frame ----
  *(--stk) = i686_GDT_USER_DATA_SEGMENT; // user SS
  *(--stk) = PROCESS_STACK_TOP;          // user ESP
  *(--stk) = 0x202;                      // EFLAGS
  *(--stk) = i686_GDT_USER_CODE_SEGMENT; // CS
  *(--stk) = (uint32_t)entry;            // EIP

  // for popa before iret
  *(--stk) = 1; // eax
  *(--stk) = 2; // ecx
  *(--stk) = 3; // edx
  *(--stk) = 4; // ebx
  *(--stk) = 5; // esp dummy
  *(--stk) = 6; // ebp
  *(--stk) = 7; // esi
  *(--stk) = 8; // edi

  *(--stk) = i686_GDT_KERNEL_DATA_SEGMENT; // ds

  // finally set PCB
  p->kernel_esp = stk;
}
