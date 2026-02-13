#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <stddef.h>
#include <stdint.h>

void kbd_init();
int kbd_read(char *buf, size_t count);

#endif
