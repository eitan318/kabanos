#include "arch/i686/gdt.h"
#include "arch/types.h"
#include "hal.h"
#include "sched/sched.h"

void hal_thread_save(arch_thread_t *thread, void *context) {
  thread->kernel_esp = context;
}

extern void __attribute__((naked))
thread_switch_to(void **old_esp, void *new_esp, uint32_t cr3);

void hal_thread_switch(thread_t *current, thread_t *next) {
  void **old_esp;
  thread_switch_to(old_esp, next->arch->kernel_esp,
                   next->process->vmspace->arch->pd_phys);
  if (current) {
    current->arch->kernel_esp = old_esp;
  }
}

// Build initial interrupt frame on kernel stack, should match isr_common
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
    *(--sp) = 0xDD; // EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
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
