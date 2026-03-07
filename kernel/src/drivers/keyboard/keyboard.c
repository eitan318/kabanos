#include "drivers/keyboard/keyboard.h"
#include "adt/circular_buffer.h"
#include "device.h"
#include "drivers/console/console.h"
#include "hal.h"
#include "isr.h"
#include "klib/stdio.h"
#include "klib/string.h"
#include "modules.h"
#include "sched/wait.h"
#include "spinlock.h"

#define KBD_IRQ 1
#define KBD_INT 0x21

#define KBD_SHIFT 0x2A
#define KBD_SHIFT_R 0x36
#define KBD_CTRL 0x1D
#define KBD_BACKSPACE 0x8

#define MAX_PRESS_SCANCODE 0x80
#define KEYBOARD_PORT 0x60

circular_buff_t g_keyboard_buff;
spinlock_t g_keyboard_lock;

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

typedef struct {
  char buf[256];
  int len;
  int read_pos; /* how far read() has consumed */
} tty_t;

static tty_t g_tty;

static void keyboard_isr_handler(trap_frame_t *regs) {
  device_t *dev = get_device_by_handle(DEVICE_HANDLE_KEYBOARD);
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
    // F1 = 0x3B, F2 = 0x3C, F3 = 0x3D
    if (keycode >= 0x3B && keycode <= 0x3F) {
      int target_workspace = keycode - 0x3B;
      // active_workspace = target_workspace;
      // refresh_screen();

      goto eoi; // Don't process this key as text
    }
    char key_ascii = shift_pressed ? scancode_to_ascii_shift[keycode]
                                   : scancode_to_ascii[keycode];
    if (ctrl_pressed && key_ascii >= 'a' && key_ascii <= 'z')
      key_ascii = key_ascii - 'a' + 1;

    if (key_ascii) {
      spinlock_acquire(&g_keyboard_lock);

      if (key_ascii == '\b' || key_ascii == 127) {
        if (g_tty.len > 0) {
          g_tty.len--;
          con_backspace();
        }
      } else {
        con_putc(key_ascii);
        if (g_tty.len < (int)sizeof(g_tty.buf) - 1)
          g_tty.buf[g_tty.len++] = key_ascii;
        if (key_ascii == '\n')
          circular_buff_enqueue(&g_keyboard_buff, (void *)1);
        wake_up_queue(&dev->wait_queue);
      }

      spinlock_release(&g_keyboard_lock);
    }
  }
eoi:
  hal_irq_send_eoi(KBD_IRQ);
}

static ssize_t tty_read(device_t *dev, void *buf, size_t count) {
  char *out = (char *)buf;
  size_t n = 0;

  spinlock_acquire(&g_keyboard_lock);

  // Wait until at least one full line (marked by a newline in ISR) is ready
  while (g_tty.len == 0 || g_tty.buf[g_tty.len - 1] != '\n') {
    if (circular_buff_is_empty(&g_keyboard_buff)) {
      wait_on_queue(&dev->wait_queue, &g_keyboard_lock);
    } else {
      // If the buffer wasn't empty but len is 0, someone
      // consumed the signal but not the data. Just clear it.
      circular_buff_dequeue(&g_keyboard_buff);
    }
  }

  // Serve the data that is already in g_tty.buf
  while (n < count && g_tty.read_pos < g_tty.len) {
    out[n++] = g_tty.buf[g_tty.read_pos++];
  }

  // Reset positions if we consumed the whole line
  if (g_tty.read_pos >= g_tty.len) {
    g_tty.len = 0;
    g_tty.read_pos = 0;
    // Clear the "line ready" signal we just processed
    circular_buff_dequeue(&g_keyboard_buff);
  }

  spinlock_release(&g_keyboard_lock);
  return (ssize_t)n;
}
static struct device_ops kbd_ops = {.read = tty_read, .write = NULL};

int kbd_init(module_t *module) {
  g_tty.len = 0;
  g_tty.read_pos = 0;
  circular_buff_init(&g_keyboard_buff);
  g_keyboard_lock = (spinlock_t)SPINLOCK_RELEASED;
  isr_handler_register(KBD_INT, keyboard_isr_handler);
  hal_irq_enable(KBD_IRQ);

  device_t *dev = device_init(DEVICE_HANDLE_KEYBOARD);
  dev->ops = &kbd_ops;
  return 0;
}

static const char *kbd_deps[] = {"hal", "devices", NULL};

ITER_MODULE(keyboard) = {
    .name = "keyboard",
    .required_modules_names = kbd_deps,
    .init = &kbd_init,
    .fini = NULL,
};
