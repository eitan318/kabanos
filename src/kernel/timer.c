#include "timer.h"
#include "isr.h"

void timer_isr_handler(struct regs *regs) {
	// Timer tick handler (IRQ 0)
    // This is called on every timer interrupt

    // Send End of Interrupt (EOI) to acknowledge the IRQ
    hal_irq_send_eoi(TIMER_IRQ);
	
	asm volatile("int $45");
}

void timer_init() {
  // Enable keyboard interrupt (IRQ0)
  hal_irq_enable(TIMER_IRQ);
  isr_handler_register(TIMER_INTERRUPT, timer_isr_handler);
}
