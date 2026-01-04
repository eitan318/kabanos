#include "task.h"
#include "arch/i686/gdt.h"
#include "memory_management/paging.h"
#include "memory_management/va_allocation.h"

extern PageDirectory *g_kernel_page_dir;

// Default memory layout for processes
#define PROCESS_STACK_TOP 0xBFFFF000 // Just below 3GB
#define PROCESS_STACK_SIZE 0x2000    // 8KB
#define PROCESS_HEAP_START 0x8000000 // Just below 3GB
#define PROCESS_HEAP_SIZE 0x2000     // 8KB

#define PROCESS_KERNEL_STACKS_START 0x9000000
#define PROCESS_KERNEL_STACKS_SIZE 0x2000

void task_setup(TCB *t, void (*entry)(void)) {
  PageDirectory *page_dir = paging_create();

  // Allocate user stack
  va_alloc_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE,
                  PAGE_USER | PAGE_WRITABLE, true);

  // Allocate user heap
  va_alloc_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE,
                  PAGE_USER | PAGE_WRITABLE, false);

  // --- Allocate kernel stack for process ---
  uint32_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACKS_SIZE;

  va_alloc_region(g_kernel_page_dir, kernel_stack_bottom,
                  PROCESS_KERNEL_STACKS_SIZE, PAGE_WRITABLE, false);

  t->kernel_stack_top =
      (uint32_t *)(kernel_stack_bottom + PROCESS_KERNEL_STACKS_SIZE);

  // Map same physical frame into the process page table
  for (uint32_t off = 0; off < PROCESS_KERNEL_STACKS_SIZE; off += PAGE_SIZE) {
    uint32_t va = kernel_stack_bottom + off;
    uint32_t phys = paging_get_physical(g_kernel_page_dir, va);
    paging_map(page_dir, va, phys, PAGE_WRITABLE);
  }

  // ---- iret frame ----
  uint32_t *stk = t->kernel_stack_top;

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
