#include <stdint.h>

void _start(void) {
  uint16_t port = 0x3F8; // COM1
  uint8_t c = 'A';
  for (;;) {
    asm volatile("outb %0, %1"
                 :
                 : "a"(c), "d"(port) // value in AL, port in DX
    );

    asm volatile("int $45");
  }
}
