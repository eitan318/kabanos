#include "com1.h"
#include "hal/io.h"

void com1_putc(const char c) { io_write8(0x3f8, c); }
