#include "hal.h"
#include "klib/stdio.h"

void __attribute__((noreturn)) panic_halt(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  kvfprintf(VFS_FD_STDOUT, fmt, ap);
  va_end(ap);

  hal_interrupts_disable();
  for (;;)
    hal_halt();
}

void panic_from_regs(struct trap_frame *regs) {
  int max_regs = hal_regs_max_get();
  const char *names[max_regs];
  uintptr_t vals[max_regs];

  int n = hal_describe_trap_frame(regs, max_regs, names, vals);
  for (int i = 0; i < n; i++) {
    kdebugf_and_printf("%s: 0x%lx, ", names[i], vals[i]);
  }
  kdebugf_and_printf("\n");

  uintptr_t pc = hal_regs_pc(regs);

  if (hal_regs_from_user(regs)) {
    kdebugf("User space exception at pc=0x%lx\n", pc);
    kdebugf("Cannot trace user stack from kernel\n");
    return;
  }

  kdebugf("FAULTING_INSTRUCTION_OF_PANIC: 0x%lx\n", pc);
  kdebugf("STACK_OF_PANIC:");

  uintptr_t state = 0;
  uintptr_t backtrace_pc;

  while ((backtrace_pc = hal_backtrace(&state, regs)) != 0) {
    kdebugf(" 0x%lx", backtrace_pc);
  }

  kdebugf("\n");
  panic_halt("");
}
