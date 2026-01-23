#include "hal.h"
#include <stddef.h>
#include <stdint.h>

#define weak __attribute__((__weak__))

// Init
void weak hal_arch_init(void) {}

// Panic
int weak hal_describe_regs(struct regs *regs, int max, const char **names,
                           uintptr_t *values) {
  return -1;
}
uintptr_t weak hal_backtrace(uintptr_t *data, struct regs *regs) {
  return NULL;
}

void weak hal_halt(void) {}
void weak hal_trap() {}

// Regs
int weak hal_interrupt_number(struct regs *regs) { return -1; }
uintptr_t weak hal_regs_pc(struct regs *regs) { return NULL; }
bool weak hal_regs_from_user(const struct regs *regs) { return false; }

// Interrupts
void weak hal_interrupts_enable(void) {}
void weak hal_interrupts_disable(void) {}
int weak hal_interrupts_state_get() { return -1; }

// Debug
void weak hal_serial_putc(const char c) {}

// IO
uint8_t weak io_read8(uint16_t port) { return -1; }
uint16_t weak io_read16(uint16_t port) { return -1; }
void weak io_write8(uint16_t port, uint8_t value) {}
void weak io_write16(uint16_t port, uint16_t value) {}

// IRQ
void weak hal_irq_enable(int irq) {}
void weak hal_irq_disable(int irq) {}
void weak hal_irq_send_eoi(uint8_t irq) {}

// Processes
void weak hal_set_kernel_stack(int cpu_id, void *kstack_top) {}
void *weak hal_build_initial_frame(void *kstack_top, uintptr_t entry,
                                   uintptr_t user_stack, enum thread_mode mode,
                                   int interrupt_number) {
  return NULL;
}
