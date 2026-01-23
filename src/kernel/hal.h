#pragma once
#include <stdbool.h>
#include <stdint.h>

struct regs;
typedef void (*interrupt_handler_t)(struct regs *r);

// Init
void hal_arch_init(void);

// Panic
int hal_describe_regs(struct regs *regs, int max, const char **names,
                      uintptr_t *values);
uintptr_t hal_backtrace(uintptr_t *data, struct regs *regs);
void hal_halt(void);
void hal_trap();

// Regs
int hal_interrupt_number(struct regs *regs);
uintptr_t hal_regs_pc(struct regs *regs);
bool hal_regs_from_user(const struct regs *regs);

// Interrupts
void hal_interrupts_enable(void);
void hal_interrupts_disable(void);
int hal_interrupts_state_get();

// Debug
void hal_serial_putc(const char c);

// IO
uint8_t io_read8(uint16_t port);
uint16_t io_read16(uint16_t port);
void io_write8(uint16_t port, uint8_t value);
void io_write16(uint16_t port, uint16_t value);

// IRQ
void hal_irq_enable(int irq);
void hal_irq_disable(int irq);
void hal_irq_send_eoi(uint8_t irq);

// Processes
enum thread_mode { THREAD_MODE_KERNEL, THREAD_MODE_USER };
void hal_set_kernel_stack(int cpu_id, void *kstack_top);
void *hal_build_initial_frame(void *kstack_top, uintptr_t entry,
                              uintptr_t user_stack, enum thread_mode mode,
                              int interrupt_number);
