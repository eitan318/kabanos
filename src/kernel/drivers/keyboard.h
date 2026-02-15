#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <stddef.h>
#include <stdint.h>

int kbd_read(char *buf, size_t count);

#endif
