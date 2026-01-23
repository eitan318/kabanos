#include "arch/i686/regs.h"
#include "hal.h"
#include "stdio.h"

void __attribute__((noreturn)) panic_halt(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(VFS_FD_STDOUT, fmt, ap);
  va_end(ap);

  hal_interrupts_disable();
  for (;;)
    hal_halt();
}

void panic_from_regs(struct regs *regs) {
  const char *names[14];
  uintptr_t vals[14];

  int n = hal_describe_regs(regs, 14, names, vals);
  for (int i = 0; i < n; i++) {
    printf("%s: 0x%lx, ", names[i], vals[i]);
  }
  printf("\n");

  uintptr_t pc = hal_regs_pc(regs);

  if (hal_regs_from_user(regs)) {
    debugf("User space exception at pc=0x%lx\n", pc);
    debugf("Cannot trace user stack from kernel\n");
    return;
  }

  debugf("FAULTING_INSTRUCTION_OF_PANIC: 0x%lx\n", pc);
  debugf("STACK_OF_PANIC:");

  uintptr_t state = 0;
  uintptr_t backtrace_pc;

  while ((backtrace_pc = hal_backtrace(&state, regs)) != 0) {
    printf(" 0x%lx", backtrace_pc);
  }

  printf("\n");
}
