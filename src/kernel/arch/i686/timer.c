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

void i686_timer_init(void) { isr_handler_register(TIMER_INT, timer_isr); }
