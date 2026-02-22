#include "arch/i686/gdt.h"
#include "arch/types.h"
#include "hal.h"
#include "memory_management/memdefs.h"
#include "sched/sched.h"
#include "string.h"

extern void __attribute__((naked))
thread_switch_to(void **curr_kernel_esp, void *next_kernel_esp,
                 uint32_t next_pd_phys);

void hal_thread_switch(thread_t *curr, thread_t *next) {
  uint32_t target_pd_phys =
      next->process != NULL ? next->process->vmspace->arch->pd_phys : 0;

  thread_switch_to(&curr->arch->kernel_esp, next->arch->kernel_esp,
                   target_pd_phys);
}

static trap_frame_t *thread_get_trap_frame(thread_t *thread) {
  return (trap_frame_t *)((uintptr_t)thread->kstack_top - sizeof(trap_frame_t));
}

int hal_thread_init(thread_t *t, uintptr_t entry, uintptr_t user_stack) {
  trap_frame_t *tf = thread_get_trap_frame(t);
  memset(tf, 0, sizeof(trap_frame_t));

  tf->eip = entry;
  tf->eflags = 0x202; // IF=1

  if (t->mode == THREAD_MODE_USER) {
    tf->cs = i686_GDT_USER_CS_SEL;
    tf->ss_user = i686_GDT_USER_DS_SEL;
    tf->esp_user = user_stack;
    uint32_t ds = i686_GDT_USER_DS_SEL;
    tf->ds = tf->es = tf->fs = tf->gs = ds;
  } else {
    tf->cs = i686_GDT_KERNEL_CS_SEL;
    // TODO
    // In kernel mode, the hardware doesn't pop SS/ESP on IRET
    // Some designs use a smaller struct for kernel threads
  }

  t->arch->kernel_esp = (void *)tf;

  return 0;
}

void hal_thread_set_return_value(thread_t *t, uint64_t val) {
  trap_frame_t *tf = thread_get_trap_frame(t);
  tf->eax = val;
}

int hal_thread_clone(thread_t *src, thread_t *child) {
  // clone stack mem
  void *src_kstack_base =
      (void *)((uintptr_t)src->kstack_top - PROCESS_KERNEL_STACK_SIZE);
  void *child_kstack_base =
      (void *)((uintptr_t)child->kstack_top - PROCESS_KERNEL_STACK_SIZE);

  memcpy(child_kstack_base, src_kstack_base, PROCESS_KERNEL_STACK_SIZE);

  // Locate the trap frame on the NEW stack
  // We find where the trap_frame lives relative to the top of the stack
  uintptr_t tf_offset =
      (uintptr_t)src->kstack_top - (uintptr_t)thread_get_trap_frame(src);
  trap_frame_t *child_tf =
      (trap_frame_t *)((uintptr_t)child->kstack_top - tf_offset);

  // ret val to 0 for child
  child_tf->eax = 0;

  child->arch->kernel_esp = (void *)child_tf;

  return 0;
}
