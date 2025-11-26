#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <stddef.h>
#include <stdint.h>

// Keyboard is IRQ1, which maps to interrupt 0x21 after PIC remap
#define KBD_IRQ 1
#define KBD_INT 0x21

// Modifier keys
#define KBD_SHIFT 0x2A
#define KBD_SHIFT_R 0x36
#define KBD_CTRL  0x1D

void kbd_init();
char kbd_char_get();

#endif
