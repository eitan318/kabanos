#include "isr.h"
#include "hal.h"
#include "panic.h"
#include "stdio.h"

#define ISR_HANDLERS 255

interrupt_handler_t g_isr_handlers[ISR_HANDLERS];

void isr_handler_register(uint32_t interrupt_num, interrupt_handler_t handler) {
  g_isr_handlers[interrupt_num] = handler;
}

void isr_dispatch(struct arch_regs *regs) {
  int hal_regs_interrupt_number(struct arch_regs * regs);
  uintptr_t hal_regs_pc(struct arch_regs * regs);
  int interrupt_num = hal_regs_interrupt_number(regs);
  if (g_isr_handlers[interrupt_num]) {
    g_isr_handlers[interrupt_num](regs);
    return;
  }

  if (interrupt_num >= 32) {
    debugf_and_printf("Unhandled IRQ %d!\n", interrupt_num);
    return;
  }

  debugf_and_printf("Unhandled exception %d %s\n", interrupt_num,
                    hal_exception_name(interrupt_num));

  panic_from_regs(regs);
}
