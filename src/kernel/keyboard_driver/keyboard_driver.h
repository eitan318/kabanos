#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <stddef.h>
#include <stdint.h>

// Keyboard is IRQ1, which maps to interrupt 0x21 after PIC remap
#define KBD_IRQ 1
#define KBD_INT 0x21

void kbd_init();
char kbd_char_get();

#endif