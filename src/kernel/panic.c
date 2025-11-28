#include "panic.h"
#include "arch/i686/panic.h"
#include "include/stdio.h" // kernel printf implementation
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

static void panic_backtrace(void) {
  uintptr_t *ebp;
  __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));

  printf("Backtrace (EBP chain):\n");
  int depth = 0;
  while (ebp && depth < 32) {
    uintptr_t ret = ebp[1];
    if (ret == 0)
      break;
    printf("  #%d: 0x%08x\n", depth, (unsigned)ret);
    ebp = (uintptr_t *)ebp[0];
    depth++;
  }
}

void kernel_panic() {
  // Print header
  printf("\n\n*** KERNEL PANIC ***\n");

  panic_backtrace();

  arch_panic_halt();
}
