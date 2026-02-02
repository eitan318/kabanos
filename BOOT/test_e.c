#include <stddef.h>

void _start(void) {
  for (;;) {
    char *str = "E";
    size_t len = 0;
    while (str[len]) {
      len++;
    }
    asm volatile("int $0x80" : : "a"(1), "b"(str), "c"(len) : "memory");
    asm volatile("int $0x45");
  }
}
