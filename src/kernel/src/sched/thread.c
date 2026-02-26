#include "sched/thread.h"
#include "arch/types.h"
#include "hal.h"
#include "mm/kmalloc.h"
#include "mm/memdefs.h"
#include "mm/va_allocation.h"
#include "mm/vmspace.h"
#include "sched/sched.h"
#include "stdio.h"
#include "string.h"

static uint32_t next_tid = 1;
static uint32_t alloc_tid() { return next_tid++; }

// Allocate and map kernel stack (in both kernel and user page directories)
static void *alloc_kernel_stack(uint32_t tid, arch_vm_t *user_vm) {
  vaddr_t stack_bottom =
      PROCESS_KERNEL_STACKS_START + tid * PROCESS_KERNEL_STACK_SIZE;
  vaddr_t stack_top = stack_bottom + PROCESS_KERNEL_STACK_SIZE;

  extern vmspace_t *g_kernel_vmspace;

  /* Allocate in kernel page directory */
  if (!va_alloc_region(g_kernel_vmspace->arch, stack_bottom,
                       PROCESS_KERNEL_STACK_SIZE, PAGE_READWRITE)) {
    kdebugf("Failed to alloc kernel stack for tid %u\n", tid);
    return NULL;
  }

  /* Map same physical pages into user page directory if provided */
  if (user_vm) {
    for (vaddr_t va = stack_bottom; va < stack_top; va += PAGE_SIZE) {
      paddr_t phys = hal_vm_virt_to_phys(g_kernel_vmspace->arch, va);
      if (!phys || !hal_vm_map(user_vm, va, phys, PAGE_READWRITE)) {
        kdebugf("Failed to map kernel stack into user PD at 0x%x\n", va);
        va_free_region(g_kernel_vmspace->arch, stack_bottom,
                       PROCESS_KERNEL_STACK_SIZE);
        return NULL;
      }
    }
  }

  return (void *)stack_top;
}

thread_t *thread_create(process_t *proc, uintptr_t entry, uintptr_t user_stack,
                        enum thread_mode mode, enum thread_priority p) {
  thread_t *t = kmalloc(sizeof(*t));
  if (!t)
    return NULL;

  memset(t, 0, sizeof(*t));
  t->tid = alloc_tid();
  t->process = proc;
  t->state = THREAD_NEW;
  t->priority = p;
  t->curr_time_quantum_ticks_passed = 0;
  t->curr_time_quantum = 0;

  t->mode = mode;

  // Allocate the arch-specific part (if it's a pointer)
  t->arch = kmalloc(sizeof(*t->arch));
  if (!t->arch) {
    kfree(t);
    return NULL;
  }

  // 1. Allocate kernel stack (still generic logic)
  arch_vm_t *user_vm =
      (mode == THREAD_MODE_USER && proc) ? proc->vmspace->arch : NULL;
  void *kstack_top = alloc_kernel_stack(t->tid, user_vm);
  if (!kstack_top) {
    kfree(t->arch);
    kfree(t);
    return NULL;
  }
  t->kstack_top = kstack_top;

  if (hal_thread_init(t, entry, user_stack) != 0) {
    kfree(t->arch);
    kfree(t);
    return NULL;
  }

  return t;
}

thread_t *thread_create_user(process_t *proc, uintptr_t entry,
                             uintptr_t user_stack, enum thread_priority p) {
  return thread_create(proc, entry, user_stack, THREAD_MODE_USER, p);
}

thread_t *thread_create_kernel(process_t *proc, uintptr_t entry) {
  return thread_create(proc, entry, 0, THREAD_MODE_KERNEL, PRIORITY_HIGH);
}

thread_t *thread_clone(thread_t *src, process_t *dst_proc) {
  // 1. Allocate the thread structure
  thread_t *child = kmalloc(sizeof(thread_t));
  if (!child)
    return NULL;

  // 2. Copy the TCB (Thread Control Block)
  memcpy(child, src, sizeof(thread_t));

  // 3. Customize unique identifiers
  child->tid = alloc_tid();
  child->process = dst_proc; // Point to the NEW process (and its CR3)
  child->state = THREAD_NEW; // Scheduler will move it to READY

  // Reset scheduler stats so the child doesn't inherit parent's "tiredness"
  child->curr_time_quantum_ticks_passed = 0;
  child->next = NULL;
  child->next_sleep = NULL;

  // 4. Allocate a new architecture-specific struct
  child->arch = kmalloc(sizeof(*child->arch));
  if (!child->arch) {
    kfree(child);
    return NULL;
  }

  // 5. Allocate a new kernel stack
  arch_vm_t *user_vm = (dst_proc) ? dst_proc->vmspace->arch : NULL;
  void *child_kstack_top = alloc_kernel_stack(child->tid, user_vm);
  if (!child_kstack_top) {
    kfree(child->arch);
    kfree(child);
    return NULL;
  }
  child->kstack_top = child_kstack_top;

  // 6. ARCH-SPECIFIC COPY
  if (hal_thread_clone(src, child) != 0) {
    kdebugf("Fork faild to clone curr thread");
    return NULL;
  }

  return child;
}

void thread_destroy(thread_t *t) {
  if (!t)
    return;

  t->state = THREAD_DEAD;
  sched_dequeue(t);

  if (t->kstack_top) {
    vaddr_t stack_bottom = (vaddr_t)t->kstack_top - PROCESS_KERNEL_STACK_SIZE;
    extern vmspace_t *g_kernel_vmspace;
    va_free_region(g_kernel_vmspace->arch, stack_bottom,
                   PROCESS_KERNEL_STACK_SIZE);

    /* Also unmap from user PD if it was a user thread */
    if (t->mode == THREAD_MODE_USER && t->process &&
        t->process->vmspace->arch) {
      for (vaddr_t va = stack_bottom; va < (vaddr_t)t->kstack_top;
           va += PAGE_SIZE) {
        hal_vm_unmap(t->process->vmspace->arch, va);
      }
    }
  }

  kfree(t);
}
