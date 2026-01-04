#include "task.h"
#include "arch/i686/gdt.h"
#include "arch/i686/isr/isr.h"
#include "include/stdio.h"
#include "memory_management/frame_allocator.h"
#include "memory_management/paging.h"
#include "memory_management/va_allocation.h"

extern PageDirectory *g_kernel_page_dir; // global

bool allocate_user_stack(uint32_t stack_top, uint32_t stack_size,
                         PageDirectory *page_dir) {
  if (!page_dir || stack_size == 0) {
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

  return (uint32_t *)stack_top;

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
  return NULL;
}

void setup_task(TCB *t, void (*entry)(void)) {
  PageDirectory *page_dir = paging_create();

  allocate_user_stack(PROCESS_STACK_TOP, PROCESS_STACK_SIZE, page_dir);
  uint32_t *kernel_stack = va_alloc(KERNEL_STACK_SIZE, true);

  // Map same physical frame into the process page table
  uint32_t phys =
      paging_get_physical(g_kernel_page_dir, (uint32_t)kernel_stack);
  paging_map(page_dir, (uint32_t)kernel_stack, phys, PAGE_WRITABLE);

  uint32_t *kernel_stack_top =
      (uint32_t *)((uint8_t *)kernel_stack + KERNEL_STACK_SIZE);
  t->kernel_stack_top = kernel_stack_top;
  uint32_t *stk = kernel_stack_top;

  // ---- iret frame ----
  //
  if (t->mode == TASK_MODE_USER) {
    *(--stk) = i686_GDT_USER_DS_SEL;
    *(--stk) = (uint32_t)t->user_esp;
    *(--stk) = 0x202;
    *(--stk) = i686_GDT_USER_CS_SEL;
    *(--stk) = (uint32_t)entry;
  } else {
    *(--stk) = 0x202;                  // EFLAGS
    *(--stk) = i686_GDT_KERNEL_CS_SEL; // CS
    *(--stk) = (uint32_t)entry;        // EIP
  }

  // ---- interrupt frame ----
  *(--stk) = 0;              // error
  *(--stk) = PREEMPTIVE_INT; // int number

  // ---- saved DS ----
  *(--stk) = i686_GDT_KERNEL_DS_SEL;

  // ---- pusha frame ----
  *(--stk) = 0; // edi
  *(--stk) = 0; // esi
  *(--stk) = 0; // ebp
  *(--stk) = 0; // esp dummy
  *(--stk) = 0; // ebx
  *(--stk) = 0; // edx
  *(--stk) = 0; // ecx
  *(--stk) = 0; // eax

  t->kernel_esp = stk;
  t->cr3 = (uint32_t)page_dir;
}
