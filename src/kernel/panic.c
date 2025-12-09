#include "panic.h"
#include "arch/i686/panic.h"
#include "include/stdio.h" // kernel printf implementation
#include "kernel_symbols.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

static void panic_ebp_backtrace(uintptr_t *ebp) {
  printf("Backtrace (EBP chain):\n");
  debugf("Backtrace (EBP chain):\n");

  int depth = 0;
  uintptr_t *cur_ebp = ebp; // keep a copy for tool output
  while (cur_ebp && depth < 32) {
    uintptr_t ret = cur_ebp[1];
    if (ret == 0)
      break;
    printf("  #%d: 0x%x <%s>\n", depth, (unsigned)ret, lookup_symbol(ret));
    debugf("  #%d: 0x%x <%s>\n", depth, (unsigned)ret, lookup_symbol(ret));
    cur_ebp = (uintptr_t *)cur_ebp[0];
    depth++;
  }
}

// for than the run.py to use with addr2line tool
void debugf_stacktrace_line(uintptr_t *ebp) {
  debugf("STACK_OF_PANIC[123]:");
  int depth = 0;
  uintptr_t *cur_ebp = ebp; // keep a copy for tool output
  while (cur_ebp && depth < 32) {
    uintptr_t ret = cur_ebp[1];
    if (ret == 0)
      break;
    debugf(" 0x%x", (unsigned)ret);
    cur_ebp = (uintptr_t *)cur_ebp[0];
    depth++;
  }
  debugf("\n");
}

void kernel_panic(uintptr_t fault_eip) {
  debugf("FAULTING_INSTRUCTION_OF_PANIC[123]: 0x%x\n", (unsigned)fault_eip);

  uintptr_t *ebp;
  __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
  debugf_stacktrace_line(ebp);
  __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
  panic_ebp_backtrace(ebp);

  arch_panic_halt();
}
