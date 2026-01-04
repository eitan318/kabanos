#include "task.h"
#include "arch/i686/gdt.h"
#include "kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/paging.h"
#include "memory_management/va_allocation.h"
#include <stdbool.h>
#include <stdlib.h>

extern PageDirectory *g_kernel_page_dir;

// Default memory layout for processes
static int next_pid = 1;

void task_kill(TCB *t) {
  // Switch to kernel page dir
  page_dir_load((uint32_t)g_kernel_page_dir);
  PageDirectory *page_dir = (PageDirectory *)t->cr3;

  // Free user stack
  va_free_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, true);

  // Free user heap
  va_free_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE, false);

  uint32_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACKS_SIZE;

  va_free_region(g_kernel_page_dir, kernel_stack_bottom,
                 PROCESS_KERNEL_STACKS_SIZE, false);
  paging_destroy(page_dir);

  t->state = TASK_STATE_KILLED;
  kfree(t);
}

// TCB *task_setup(void (*entry)(void), TaskMode mode) {

void task_setup(TCB *t, void (*entry)(void)) {
  // TCB *t = kmalloc(sizeof(*t));
  t->pid = next_pid++;
  t->state = TASK_STATE_NEW;
  // t->mode = mode;
  PageDirectory *page_dir = paging_create_kernel();
  t->cr3 = (uint32_t)page_dir;
  t->user_esp = (uint32_t *)(PROCESS_STACK_TOP + PROCESS_STACK_SIZE);

  // Allocate user stack
  if (!va_alloc_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE,
                       PAGE_USER | PAGE_WRITABLE, true)) {
    paging_destroy(page_dir);

    // return NULL;
    return;
  }

  // Allocate user heap
  if (!va_alloc_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE,
                       PAGE_USER | PAGE_WRITABLE, false)) {
    va_free_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, true);
    paging_destroy(page_dir);
    // return NULL;
    return;
  }

  // --- Allocate kernel stack for process ---
  uint32_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACKS_SIZE;

  if (!va_alloc_region(g_kernel_page_dir, kernel_stack_bottom,
                       PROCESS_KERNEL_STACKS_SIZE, PAGE_WRITABLE, false)) {
    va_free_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, true);
    va_free_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE, false);
    paging_destroy(page_dir);
    // return NULL;
    return;
  }

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

  // return t;
}
