#include "e9.h"
#include "hal/io.h"

void e9_putc(const char c) { io_write8(0xE9, c); }
