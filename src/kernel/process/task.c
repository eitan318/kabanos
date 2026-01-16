#include "task.h"
#include "arch/i686/gdt.h"
#include "elf/elf.h"
#include "include/stdio.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmm.h"
#include <stdbool.h>
#include <stdlib.h>

extern vmspace_t *g_kernel_vmspace;

// Default memory layout for processes
static int next_pid = 0xB;

void task_kill(TCB *t) {
  // Switch to kernel page dir
  vmspace_switch(g_kernel_vmspace);
  page_dir_t *page_dir = (page_dir_t *)t->cr3;

  // Free user stack
  va_free_region(page_dir, USER_STACK_BOTTOM, USER_STACK_SIZE);

  uint32_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACK_SIZE;

  va_free_region(g_kernel_vmspace->pd, kernel_stack_bottom,
                 PROCESS_KERNEL_STACK_SIZE);

  vmspace_t *vmspace = t->vmspace;
  vmspace_destroy(vmspace);

  t->state = TASK_STATE_KILLED;
  kfree(t);
}

void task_setup(TCB *t, void (*entry)(void), TaskMode mode) {
  // Set PID FIRST
  t->pid = next_pid++;
  t->state = TASK_STATE_NEW;
  t->mode = mode;

  // --- Allocate kernel stack for process --- (Has to be before user pd
  // creation to be included there)
  vaddr_t kernel_stack_bottom =
      PROCESS_KERNEL_STACKS_START + t->pid * PROCESS_KERNEL_STACK_SIZE;

  if (!va_alloc_region(g_kernel_vmspace->pd, kernel_stack_bottom,
                       PROCESS_KERNEL_STACK_SIZE, PAGE_READWRITE)) {
    debugf("Failed to alloc kernel stack for proc");
    return;
  }

  t->vmspace = user_vmspace_creat();
  t->cr3 = (uint32_t)t->vmspace->pd_phys;
  t->kernel_stack_top =
      (uint32_t *)(kernel_stack_bottom + PROCESS_KERNEL_STACK_SIZE);

  // Allocate user stack
  if (!va_alloc_region(t->vmspace->pd, USER_STACK_BOTTOM + 1, USER_STACK_SIZE,
                       PAGE_USER | PAGE_READWRITE)) {
    vmspace_destroy(t->vmspace);
    debugf("Failed to alloc user stack");
    return;
  }
  t->user_esp = (uint32_t *)(USER_STACK_BOTTOM + USER_STACK_SIZE);

  // ---- iret frame ----
  uint32_t *stk = t->kernel_stack_top;

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

  *(--stk) = i686_GDT_USER_DS_SEL; // ds

  t->kernel_esp = stk;
}
