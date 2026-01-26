#include <stdint.h>
#include <stddef.h>

void _start(void) {
  for (;;) {
    char *str = "Hello from process B\n";
	size_t len = 0;
	while (str[len]) {
		len++;
	}
	asm volatile(
		"int $80"
		: 
		: "a"(1), "b"(str), "c"(len)
		: "memory"
	);
    asm volatile("int $45");
  }
}