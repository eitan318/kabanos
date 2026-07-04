/**
 * @file isr.c
 * @brief Interrupt handler table and common dispatch.
 *
 * Unhandled IRQs are logged and ignored; unhandled CPU exceptions panic.
 */
#include "isr.h"
#include "hal.h"
#include "klib/stdio.h"
#include "panic.h"

#define ISR_HANDLERS 255

interrupt_handler_t g_isr_handlers[ISR_HANDLERS];

void isr_handler_register(uint32_t interrupt_num, interrupt_handler_t handler) {
  g_isr_handlers[interrupt_num] = handler;
}

void isr_dispatch(trap_frame_t *regs) {
  int interrupt_num = hal_regs_interrupt_number(regs);
  if (g_isr_handlers[interrupt_num]) {
    g_isr_handlers[interrupt_num](regs);
    return;
  }

  if (interrupt_num >= 32) {
    kdebugf_and_printf("Unhandled IRQ %d!\n", interrupt_num);
    return;
  }

  kdebugf_and_printf("Unhandled exception %d %s\n", interrupt_num,
                     hal_exception_name(interrupt_num));

  panic_from_regs(regs);
}
