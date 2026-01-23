#include "isr.h"
#define SYSCALL_INTERRUPT 0x80

void syscall_isr_handler(struct regs *regs) {}

void syscall_init() {
  isr_handler_register(SYSCALL_INTERRUPT, syscall_isr_handler);
}
