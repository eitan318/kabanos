#include "hal.h"
#include "isr.h"
#include "sched/sched.h"

#define TIMER_IRQ 0
#define TIMER_VECTOR 32

static void timer_isr(struct regs *r) {
  hal_irq_send_eoi(TIMER_IRQ); // Works only with eoi before
  sched_tick(r);
}

void i686_timer_init(void) {
  isr_handler_register(TIMER_VECTOR, timer_isr);
  hal_irq_enable(TIMER_IRQ);
}
