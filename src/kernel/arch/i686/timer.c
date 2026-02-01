#include "hal.h"
#include "isr.h"
#include "sched/sched.h"

#define TIMER_IRQ 0
#define TIMER_INT 0x20

static void timer_isr(struct arch_regs *r) {
  hal_irq_send_eoi(TIMER_IRQ); // Works only with eoi before
  sched_tick(r);
}

void hal_timer_enable() { hal_irq_enable(TIMER_IRQ); }

void i686_timer_init(void) {
  // 1. Set the PIT frequency (e.g., 100Hz)
  uint32_t frequency = 100;
  uint32_t divisor = 1193182 / frequency;

  hal_out8(0x43, 0x36);                  // Command: Square Wave Mode
  hal_out8(0x40, divisor & 0xFF);        // Low byte
  hal_out8(0x40, (divisor >> 8) & 0xFF); // High byte

  // 2. Register the handler
  isr_handler_register(TIMER_INT, timer_isr);
}
