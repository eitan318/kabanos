#include "hal.h"
void panic_from_regs(struct trap_frame *regs);
void __attribute__((noreturn)) panic_halt(const char *fmt, ...);
