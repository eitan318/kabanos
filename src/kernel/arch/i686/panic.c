#include "panic.h"

void __attribute__((noreturn)) arch_panic_halt(void) {
  __asm__ volatile("cli");
  for (;;) {
    __asm__ volatile("hlt");
  }
}
