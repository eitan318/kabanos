#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include "klib/stddef.h"
#include "klib/stdint.h"

int kbd_read(char *buf, size_t count);

#endif
