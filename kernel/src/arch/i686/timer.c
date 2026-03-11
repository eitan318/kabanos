#include "hal.h"
#include "isr.h"
#include "sched/sched.h"

#define TIMER_IRQ 0
#define TIMER_INT 0x20

static volatile uint32_t g_ticks = 0;

uint32_t timer_get_ticks(void) {        
    return g_ticks;
}

static void timer_isr(trap_frame_t *r) {
  g_ticks++;
  hal_irq_send_eoi(TIMER_IRQ); // Works only with eoi before
  sched_tick(r);
}

void hal_timer_enable() { hal_irq_enable(TIMER_IRQ); }

void i686_timer_init(uint32_t frequency_hz) {
  isr_handler_register(TIMER_INT, timer_isr);
  // 2. Calculate the divisor
  // Base frequency is 1.193182 MHz
  uint32_t divisor = 1193182 / frequency_hz;

  // The divisor must fit in 16 bits (65535).
  // A frequency of 18.2Hz is the slowest possible.
  if (divisor > 65535)
    divisor = 65535;
  if (divisor < 1)
    divisor = 1;

  // 3. Send the Command Byte to the PIT Mode/Command register (0x43)
  // Binary: 00 11 010 0
  // 00 (Channel 0) | 11 (Access low/high byte) | 010 (Mode 2: Rate Generator) |
  // 0 (Binary)
  hal_out8(0x43, 0x34);

  // 4. Send the Divisor (Low byte then High byte) to Channel 0 (0x40)
  hal_out8(0x40, (uint8_t)(divisor & 0xFF));
  hal_out8(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}
