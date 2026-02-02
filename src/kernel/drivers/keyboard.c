#include "drivers/keyboard.h"
#include "hal.h"
#include "isr.h"
#include "utils/queue.h"

// Keyboard is IRQ1, which maps to interrupt 0x21 after PIC remap
#define KBD_IRQ 1
#define KBD_INT 0x21

// Modifier keys
#define KBD_SHIFT 0x2A
#define KBD_SHIFT_R 0x36
#define KBD_CTRL 0x1D
#define KBD_BACKSPACE 0x8

#define MAX_PRESS_SCANCODE 0x80
#define KEYBOARD_PORT 0x60

Queue keyboard_queue;

// Modifier key states
static int shift_pressed = 0;
static int ctrl_pressed = 0;

// Partial US layout scancode -> ASCII
static char scancode_to_ascii[128] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' ', 0};

// Shifted characters (for US keyboard)
static char scancode_to_ascii_shift[128] = {
    0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' ', 0};

static void keyboard_isr_handler(struct arch_regs *regs) {
  uint8_t scancode = hal_in8(KEYBOARD_PORT);

  // Handle key release
  int key_released = scancode & MAX_PRESS_SCANCODE;
  uint8_t keycode = scancode & 0x7F;

  if (keycode == KBD_SHIFT || keycode == KBD_SHIFT_R) {
    shift_pressed = !key_released;
    goto eoi;
  }
  if (keycode == KBD_CTRL) {
    ctrl_pressed = !key_released;
    goto eoi;
  }

  // Only handle key press events
  if (!key_released) {
    char key_ascii = shift_pressed ? scancode_to_ascii_shift[keycode]
                                   : scancode_to_ascii[keycode];

    // Handle Ctrl combinations (optional: e.g., Ctrl+C -> 0x03)
    if (ctrl_pressed && key_ascii >= 'a' && key_ascii <= 'z') {
      key_ascii = key_ascii - 'a' + 1; // Ctrl+key -> ASCII control code
    }

    if (key_ascii) {
      enqueue(&keyboard_queue, (void *)(uintptr_t)key_ascii);
    }
  }

eoi:
  hal_irq_send_eoi(KBD_IRQ);
}

void kbd_init() {
  queue_init(&keyboard_queue);

  // Register keyboard interrupt handler
  isr_handler_register(KBD_INT, keyboard_isr_handler);
  // Enable keyboard interrupt (IRQ1)
  hal_irq_enable(KBD_IRQ);
}

char kbd_char_get() { return (char)(uintptr_t)dequeue(&keyboard_queue); }
