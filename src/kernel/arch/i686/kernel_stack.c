#include "gdt.h"
#include "hal.h"
#include "tss.h"

void hal_set_kernel_stack(int cpu_id, void *kstack_top) {
  tss_entry_t *curr_tss = tss_entry_get(cpu_id);
  curr_tss->ss0 = i686_GDT_KERNEL_DS_SEL;
  curr_tss->esp0 = (uint32_t)(kstack_top);
}
// Build initial interrupt frame on kernel stack
void *hal_build_initial_frame(void *kstack_top, uintptr_t entry,
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
  *(--sp) = (mode == THREAD_MODE_USER) ? i686_GDT_USER_DS_SEL
                                       : i686_GDT_KERNEL_DS_SEL;

  return sp;
}
