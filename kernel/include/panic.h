#include "hal.h"
void panic_from_regs(trap_frame_t *regs);
void __attribute__((noreturn)) panic(const char *fmt, ...);
