#include "syscall.h"
#include <stddef.h>

void _start(void) {
  for (;;) {
    char *str = "E";
    size_t len = 0;
    while (str[len]) {
      len++;
    }
    _syscall6(SYSCALL_NUMBERS_SYS_WRITE, str, len, 0, 0, 0, 0);
    asm volatile("int $0x45");
  }
}
