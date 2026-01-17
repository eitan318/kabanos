// test.c — bare-metal ELF
void _start(void) {
  // Your ELF “program”
  asm volatile("int $45");
  for (;;) {
  } // halt loop
}
