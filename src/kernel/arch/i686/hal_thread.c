#include "arch/i686/gdt.h"
#include "arch/types.h"
#include "hal.h"
#include "sched/sched.h"
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

int hal_thread_clone_current(thread_t *current_thread, thread_t *dest_thread) {
  uintptr_t current_esp;
  asm volatile("mov %%esp, %0" : "=r"(current_esp));

  size_t stack_used = (uintptr_t)current_thread->kstack_top - current_esp;

  // Set the destination stack pointer at the same offset
  dest_thread->arch->kernel_esp =
      (void *)((uintptr_t)dest_thread->kstack_top - stack_used);

  // Copy the live stack to the new thread's stack
  memcpy(dest_thread->arch->kernel_esp, (void *)current_esp, stack_used);

  hal_thread_set_return_value(dest_thread, 0);

  return 0;
}
