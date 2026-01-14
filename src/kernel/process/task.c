#include "task.h"
#include "arch/i686/gdt.h"
#include "elf/elf.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmm.h"
#include <stdbool.h>
#include <stdlib.h>

extern vmspace_t *g_kernel_vmspace;

// Default memory layout for processes
static int next_pid = 1;

void task_kill(TCB *t) {
  // Switch to kernel page dir
  vmspace_switch(g_kernel_vmspace);
  page_dir_t *page_dir = (page_dir_t *)t->cr3;

  // Free user stack
  va_free_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, true);

  // Free user heap
  va_free_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE, false);

  uint32_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACK_SIZE;

  va_free_region(g_kernel_vmspace->pd, kernel_stack_bottom,
                 PROCESS_KERNEL_STACK_SIZE, false);

  vmspace_t *vmspace = t->vmspace;
  vmspace_destroy(vmspace);

  t->state = TASK_STATE_KILLED;
  kfree(t);
}

void task_setup(TCB *t, void (*entry)(void), TaskMode mode) {
  vmspace_t *vmspace = user_vmspace_creat();
  page_dir_t *page_dir = vmspace->pd;
  t->pid = next_pid++;
  t->state = TASK_STATE_NEW;
  t->mode = mode;
  t->cr3 = (uint32_t)page_dir;

  // Allocate user stack
  if (!va_alloc_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE,
                       PAGE_USER | PAGE_READWRITE, true)) {
    vmspace_destroy(vmspace);
    return;
  }

  // Allocate user heap
  if (!va_alloc_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE,
                       PAGE_USER | PAGE_READWRITE, false)) {
    va_free_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, true);
    vmspace_destroy(vmspace);
    return;
  }

  // --- Allocate kernel stack for process ---
  uint32_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACK_SIZE;

  if (!va_alloc_region(g_kernel_vmspace->pd, kernel_stack_bottom,
                       PROCESS_KERNEL_STACK_SIZE, PAGE_READWRITE, false)) {
    va_free_region(page_dir, PROCESS_STACK_TOP, PROCESS_STACK_SIZE, true);
    va_free_region(page_dir, PROCESS_HEAP_START, PROCESS_HEAP_SIZE, false);
    vmspace_destroy(vmspace);
    return;
  }

  t->kernel_stack_top =
      (uint32_t *)(kernel_stack_bottom + PROCESS_KERNEL_STACK_SIZE);

  // Map same physical frame into the process page table
  for (uint32_t off = 0; off < PROCESS_KERNEL_STACK_SIZE; off += PAGE_SIZE) {
    uint32_t va = kernel_stack_bottom + off;
    uint32_t phys = virt_to_phys(g_kernel_vmspace->pd, va);
    vm_map(page_dir, va, phys, PAGE_READWRITE);
  }

  // ---- iret frame ----
  uint32_t *stk = t->kernel_stack_top;

  t->user_esp = (uint32_t *)(PROCESS_STACK_TOP);

  if (t->mode == TASK_MODE_USER) {
    *(--stk) = i686_GDT_USER_DS_SEL;  // SS
    *(--stk) = (uint32_t)t->user_esp; // ESP
    *(--stk) = 0x202;                 // EFLAGS
    *(--stk) = i686_GDT_USER_CS_SEL;  // CS
    *(--stk) = (uint32_t)entry;
  } else {
    *(--stk) = 0x202;                  // EFLAGS
    *(--stk) = i686_GDT_KERNEL_CS_SEL; // CS
    *(--stk) = (uint32_t)entry;        // EIP
  }

  // ---- interrupt frame ----
  *(--stk) = 0;              // error
  *(--stk) = PREEMPTIVE_INT; // int number

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
}
