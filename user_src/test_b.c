#include "stdio.h"
#include "syscall.h"
#include <stddef.h>

void _start(void) {
  for (;;) {
    printf("B");

    // Simple delay loop so it doesn't fill the screen too fast
    for (volatile int i = 0; i < 10000000; i++)
      ;

    // Tell the kernel: "I'm done for now, let someone else try"
    _syscall6(SYSCALL_NUMBERS_SYS_YIELD, 0, 0, 0, 0, 0, 0);
  }
}
