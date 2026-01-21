#include "com1.h"
#include "io.h"

void com1_putc(const char c) { i686_outb(0x3f8, c); }
