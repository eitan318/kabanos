#include "isr.h"
#include "hal.h"
#include "panic.h"
#include "stdio.h"

#define ISR_HANDLERS 255

interrupt_handler_t g_isr_handlers[ISR_HANDLERS];

// clang-format off
static const char *const g_exceptions[] = {
  "Divide by zero error", "Debug", "Non-maskable Interrupt", "Breakpoint",
  "Overflow", "Bound Range Exceeded", "Invalid Opcode", "Device Not Available",
  "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS",
  "Segment Not Present", "Stack-Segment Fault", "General Protection Fault",
  "Page Fault", "", "x87 Floating-Point Exception", "Alignment Check",
  "Machine Check", "SIMD Floating-Point Exception", "Virtualization Exception",
  "Control Protection", "", "", "", "", "", "",
  "Hypervisor Injection", "VMM Communication Exception", "Security Exception", ""
};
// clang-format on

void isr_handler_register(uint32_t interrupt_num, interrupt_handler_t handler) {
  g_isr_handlers[interrupt_num] = handler;
}

void isr_dispatch(struct regs *regs) {
  int hal_interrupt_number(struct regs * regs);
  uintptr_t hal_regs_pc(struct regs * regs);
  int interrupt_num = hal_interrupt_number(regs);
  if (g_isr_handlers[interrupt_num]) {
    g_isr_handlers[interrupt_num](regs);
    return;
  }

  if (interrupt_num >= 32) {
    debugf_and_printf("Unhandled IRQ %d!\n", interrupt_num);
    return;
  }

  debugf_and_printf("Unhandled exception %d %s\n", interrupt_num,
                    g_exceptions[interrupt_num]);

  panic_from_regs(regs);
}
