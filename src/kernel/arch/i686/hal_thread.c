#include "arch/i686/gdt.h"
#include "arch/types.h"
#include "hal.h"
#include "memory_management/memdefs.h"
#include "sched/sched.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

void hal_thread_set_return_value(thread_t *t, uint64_t val) {
  // 1. Locate the trap frame on the kernel stack.
  // The trap frame is usually at the very top (end) of the kstack
  // because it was pushed first when entering the kernel.
  trap_frame_t *tf = (trap_frame_t *)((uintptr_t)t->kstack_top - sizeof(*tf));

  // 2. Set the 'accumulator' register (rax/eax)
  tf->eax = val;
}
void hal_thread_set_userspace_state(thread_t *t, uintptr_t entry,
                                    uintptr_t user_stack) {
  trap_frame_t *tf = (trap_frame_t *)((uintptr_t)t->kstack_top - sizeof(*tf));

  // 2. Clear general purpose registers so the new program starts with a clean
  // slate
  tf->eax = 0;
  tf->ebx = 0;
  tf->ecx = 0;
  tf->edx = 0;
  tf->esi = 0;
  tf->edi = 0;
  tf->ebp = 0;

  // 3. Set the new entry point (where the CPU will jump)
  tf->eip = entry;

  // 4. Set the new user stack pointer
  tf->esp_user = user_stack;

  // 5. Ensure EFLAGS are sane (Interrupts enabled, bit 1 set)
  // 0x202 = (1 << 9) | (1 << 1)
  tf->eflags = 0x202;

  // 6. Set Segments to User Data/Code selectors (adjust based on your GDT)
  // Usually: Code = 0x1B (Index 3, RPL 3), Data = 0x23 (Index 4, RPL 3)
  tf->cs = i686_GDT_USER_CS_SEL;
  tf->ds = tf->es = tf->fs = tf->gs = tf->ss_user = i686_GDT_USER_DS_SEL;
}

extern void __attribute__((naked))
thread_switch_to(void **curr_kernel_esp, void *next_kernel_esp,
                 uint32_t next_pd_phys);

void hal_thread_switch(thread_t *curr, thread_t *next) {
  uint32_t target_pd_phys =
      next->process != NULL ? next->process->vmspace->arch->pd_phys : 0;

  thread_switch_to(&curr->arch->kernel_esp, next->arch->kernel_esp,
                   target_pd_phys);
}

// Build initial interrupt frame on kernel stack
void *build_initial_frame(void *kstack_top, uintptr_t entry,
                          uintptr_t user_stack, enum thread_mode mode,
                          int interrupt_number) {
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
  *(--sp) = 0;                // Error code
  *(--sp) = interrupt_number; // Interrupt number

  /* PUSHA frame */
  for (int i = 0; i < 8; i++) {
    *(--sp) = 0; // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
  }

  /* Segment registers */
  uint32_t ds = (mode == THREAD_MODE_USER) ? i686_GDT_USER_DS_SEL
                                           : i686_GDT_KERNEL_DS_SEL;
  *(--sp) = ds; // GS
  *(--sp) = ds; // FS
  *(--sp) = ds; // ES
  *(--sp) = ds; // DS

  return sp;
}

int hal_thread_init(thread_t *t, uintptr_t entry, uintptr_t user_stack) {
  void *sp = build_initial_frame(t->kstack_top, entry, user_stack, t->mode, 0);

  if (!sp)
    return -1;

  t->arch->kernel_esp = sp;
  return 0;
}

trap_frame_t *hal_get_trap_frame(thread_t *thread) {
  // The trap frame is pushed at the very top of the allocated stack page.
  // If kstack_top is the address of the end of the stack:
  return (trap_frame_t *)((uintptr_t)thread->kstack_top - sizeof(trap_frame_t));
}

int hal_thread_clone_current(thread_t *src, thread_t *child) {
  // 1. Copy the ENTIRE kernel stack.
  // Assuming STACK_SIZE is defined (e.g., 4096).
  // We copy from the bottom of the stack.
  void *src_stack_base =
      (void *)((uintptr_t)src->kstack_top - PROCESS_KERNEL_STACK_SIZE);
  void *child_stack_base =
      (void *)((uintptr_t)child->kstack_top - PROCESS_KERNEL_STACK_SIZE);

  memcpy(child_stack_base, src_stack_base, PROCESS_KERNEL_STACK_SIZE);

  // 2. Locate the trap frame on the NEW stack
  // We find where the trap_frame lives relative to the top of the stack
  uintptr_t tf_offset =
      (uintptr_t)src->kstack_top - (uintptr_t)hal_get_trap_frame(src);
  trap_frame_t *child_tf =
      (trap_frame_t *)((uintptr_t)child->kstack_top - tf_offset);

  int max_regs = hal_regs_max_get();
  const char *names[max_regs];
  uintptr_t vals[max_regs];

  int n = hal_describe_trap_frame(child_tf, max_regs, names, vals);
  for (int i = 0; i < n; i++) {
    debugf_and_printf("%s: 0x%lx, ", names[i], vals[i]);
  }
  debugf_and_printf("\n");

  // 3. CRITICAL: Set the return value for the child to 0
  // On x86, this is EAX. On ARM, it's R0.
  child_tf->eax = 0;

  // 4. Set up the child's context for the first time it is scheduled.
  // Usually, you'd set the child's kernel_esp to point to a "stub"
  // function like ret_from_fork that pops the registers and exits to userland.
  //
  child->arch->kernel_esp = (void *)child_tf;
  printf("child (tid %d) esp %p", child->tid, child->arch->kernel_esp);

  return 0;
}
