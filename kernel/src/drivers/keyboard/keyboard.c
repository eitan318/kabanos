/**
 * @file keyboard.c
 * @brief PS/2 keyboard driver: scancode to ASCII, feeds the TTY.
 */
#include "device.h"
#include "drivers/console/tty.h"
#include "hal.h"
#include "isr.h"
#include "modules.h"
#include <stdint.h>

#define KBD_IRQ 1
#define KBD_INT 0x21

#define KBD_SHIFT 0x2A
#define KBD_SHIFT_R 0x36
#define KBD_CTRL 0x1D
#define KBD_BACKSPACE 0x8

#define MAX_PRESS_SCANCODE 0x80
#define KEYBOARD_PORT 0x60

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

static void keyboard_isr_handler(trap_frame_t *regs) {
  uint8_t scancode = hal_in8(KEYBOARD_PORT);
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

  if (!key_released) {
    char key_ascii = shift_pressed ? scancode_to_ascii_shift[keycode]
                                   : scancode_to_ascii[keycode];
    if (ctrl_pressed && key_ascii >= 'a' && key_ascii <= 'z')
      key_ascii = key_ascii - 'a' + 1;

    if (key_ascii) {
      device_t *dev = get_device_by_handle(DEVICE_HANDLE_KEYBOARD);
      tty_input(dev, key_ascii);
    }
  }
eoi:
  hal_irq_send_eoi(KBD_IRQ);
}

static struct device_ops kbd_ops = {
    .read = tty_read, .write = NULL, .ioctl = tty_ioctl};

int kbd_init(module_t *module) {
  isr_handler_register(KBD_INT, keyboard_isr_handler);
  hal_irq_enable(KBD_IRQ);

  device_t *dev = device_init(DEVICE_HANDLE_KEYBOARD);
  dev->ops = &kbd_ops;
  termios_t init_conf = {.c_lflag = TTY_ICANON};
  tty_t *tty = tty_init(init_conf);
  dev->priv = tty;
  return 0;
}

static const char *kbd_deps[] = {"hal", "devices", NULL};

ITER_MODULE(keyboard) = {
    .name = "keyboard",
    .required_modules_names = kbd_deps,
    .init = &kbd_init,
    .fini = NULL,
};
