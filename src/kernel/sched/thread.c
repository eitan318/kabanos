#include "thread.h"
#include "arch/i686/gdt.h"
#include "include/memory.h"
#include "include/stdio.h"
#include "memory_management/kmalloc.h"
#include "memory_management/memdefs.h"
#include "memory_management/va_allocation.h"
#include "memory_management/vmspace.h"
#include "sched/sched.h"

static uint32_t next_tid = 1;
static uint32_t alloc_tid() { return next_tid++; }

// Allocate and map kernel stack (in both kernel and user page directories)
static void *alloc_kernel_stack(uint32_t tid, page_dir_t *user_pd) {
  vaddr_t stack_bottom =
      PROCESS_KERNEL_STACKS_START + tid * PROCESS_KERNEL_STACK_SIZE;
  vaddr_t stack_top = stack_bottom + PROCESS_KERNEL_STACK_SIZE;

  extern vmspace_t *g_kernel_vmspace;

  /* Allocate in kernel page directory */
  if (!va_alloc_region(g_kernel_vmspace->pd, stack_bottom,
                       PROCESS_KERNEL_STACK_SIZE, PAGE_READWRITE)) {
    debugf("Failed to alloc kernel stack for tid %u\n", tid);
    return NULL;
  }

  /* Map same physical pages into user page directory if provided */
  if (user_pd) {
    for (vaddr_t va = stack_bottom; va < stack_top; va += PAGE_SIZE) {
      paddr_t phys = virt_to_phys(g_kernel_vmspace->pd, va);
      if (!phys || !vm_map(user_pd, va, phys, PAGE_READWRITE)) {
        debugf("Failed to map kernel stack into user PD at 0x%x\n", va);
        va_free_region(g_kernel_vmspace->pd, stack_bottom,
                       PROCESS_KERNEL_STACK_SIZE);
        return NULL;
      }
    }
  }

  return (void *)stack_top;
}

// Build initial interrupt frame on kernel stack
static void *build_initial_frame(void *kstack_top, uintptr_t entry,
                                 uintptr_t user_stack, enum thread_mode mode) {
  uint32_t *sp = (uint32_t *)kstack_top;

  /* IRET frame */
  if (mode == THREAD_MODE_USER) {
    *(--sp) = i686_GDT_USER_DS_SEL; // SS
    *(--sp) = user_stack;           // ESP
  }
  *(--sp) = 0x202; // EFLAGS (IF=1)
  *(--sp) = (mode == THREAD_MODE_USER) ? i686_GDT_USER_CS_SEL
                                       : i686_GDT_KERNEL_CS_SEL;
  *(--sp) = entry; // EIP

  /* Interrupt frame */
  *(--sp) = 0;              // Error code
  *(--sp) = PREEMPTIVE_INT; // Interrupt number

  /* PUSHA frame */
  for (int i = 0; i < 8; i++) {
    *(--sp) = 0; // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
  }

  /* Segment registers */
  *(--sp) = (mode == THREAD_MODE_USER) ? i686_GDT_USER_DS_SEL
                                       : i686_GDT_KERNEL_DS_SEL;

  return sp;
}

// Generic thread creation (works for both kernel and user threads)
thread_t *thread_create(process_t *proc, uintptr_t entry, uintptr_t user_stack,
                        enum thread_mode mode) {
  thread_t *t = kmalloc(sizeof(*t));
  if (!t)
    return NULL;

  memset(t, 0, sizeof(*t));
  t->tid = alloc_tid();
  t->process = proc;
  t->state = THREAD_READY;
  t->mode = mode;

  /* Allocate kernel stack (mapped to user PD if user mode) */
  page_dir_t *user_pd =
      (mode == THREAD_MODE_USER && proc) ? proc->vmspace->pd : NULL;
  void *kstack_top = alloc_kernel_stack(t->tid, user_pd);
  if (!kstack_top) {
    kfree(t);
    return NULL;
  }

  /* Build interrupt frame */
  void *kernel_esp = build_initial_frame(kstack_top, entry, user_stack, mode);

  /* Set thread state */
  t->kstack_top = kstack_top;
  t->kernel_esp = kernel_esp;

  /* Add to process and scheduler */
  if (proc && !proc->main_thread) {
    proc->main_thread = t;
  }
  sched_add(t);

  return t;
}

thread_t *thread_create_user(process_t *proc, uintptr_t entry,
                             uintptr_t user_stack) {
  return thread_create(proc, entry, user_stack, THREAD_MODE_USER);
}

thread_t *thread_create_kernel(process_t *proc, uintptr_t entry) {
  return thread_create(proc, entry, 0, THREAD_MODE_KERNEL);
}

void thread_destroy(thread_t *t) {
  if (!t)
    return;

  if (t->kstack_top) {
    vaddr_t stack_bottom = (vaddr_t)t->kstack_top - PROCESS_KERNEL_STACK_SIZE;
    extern vmspace_t *g_kernel_vmspace;
    va_free_region(g_kernel_vmspace->pd, stack_bottom,
                   PROCESS_KERNEL_STACK_SIZE);

    /* Also unmap from user PD if it was a user thread */
    if (t->mode == THREAD_MODE_USER && t->process && t->process->vmspace->pd) {
      for (vaddr_t va = stack_bottom; va < (vaddr_t)t->kstack_top;
           va += PAGE_SIZE) {
        vm_unmap(t->process->vmspace->pd, va);
      }
    }
  }

  kfree(t);
}
