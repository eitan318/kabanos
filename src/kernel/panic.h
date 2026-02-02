#include "hal.h"
void panic_from_regs(struct arch_regs *regs);
void __attribute__((noreturn)) panic_halt(const char *fmt, ...);
