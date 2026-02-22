#include "drivers/keyboard.h"
#include "adt/circular_buffer.h"
#include "device.h"
#include "hal.h"
#include "isr.h"
#include "modules.h"
#include "sched/spinlock.h"
#include "sched/wait.h"
#include "string.h"

#define KBD_IRQ 1
#define KBD_INT 0x21

#define KBD_SHIFT 0x2A
#define KBD_SHIFT_R 0x36
#define KBD_CTRL 0x1D
#define KBD_BACKSPACE 0x8

#define MAX_PRESS_SCANCODE 0x80
#define KEYBOARD_PORT 0x60

circular_buff_t keyboard_buff;
spinlock_t keyboard_lock;

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
  device_t *dev = get_device_by_handle(DEVICE_HANDLE_KEYBOARD);
  uint8_t scancode = hal_in8(KEYBOARD_PORT);
  int key_released = scancode & MAX_PRESS_SCANCODE;
  uint8_t keycode = scancode & 0x7F;

  // Handle modifiers
  if (keycode == KBD_SHIFT || keycode == KBD_SHIFT_R) {
    shift_pressed = !key_released;
    goto eoi;
  }
  if (keycode == KBD_CTRL) {
    ctrl_pressed = !key_released;
    goto eoi;
  }

  // Process key press
  if (!key_released) {
    char key_ascii = shift_pressed ? scancode_to_ascii_shift[keycode]
                                   : scancode_to_ascii[keycode];
    if (ctrl_pressed && key_ascii >= 'a' && key_ascii <= 'z') {
      key_ascii = key_ascii - 'a' + 1;
    }

    if (key_ascii) {
      spinlock_acquire(&keyboard_lock); // 1. Lock
      circular_buff_enqueue(
          &keyboard_buff,
          (void *)(uintptr_t)key_ascii); // 2. Add data (no _unlocked!)
      wake_up_queue(&dev->wait_queue);   // 3. Wake waiting threads
      spinlock_release(&keyboard_lock);  // 4. Unlock
    }
  }

eoi:
  hal_irq_send_eoi(KBD_IRQ);
}

int kbd_read(char *buf, size_t count) {
  device_t *dev = get_device_by_handle(DEVICE_HANDLE_KEYBOARD);
  size_t i = 0;

  while (i < count) {
    spinlock_acquire(&keyboard_lock); // 1. Lock

    while (keyboard_buff.count == 0) { // 2. Check if empty
      // 3. Sleep (releases keyboard_lock, wakes up, re-acquires it)
      wait_on_queue(&dev->wait_queue, &keyboard_lock);
    }

    // 4. Now we have data AND the lock
    buf[i++] =
        (char)(uintptr_t)circular_buff_dequeue(&keyboard_buff); // No _unlocked!

    spinlock_release(&keyboard_lock); // 5. Unlock
  }

  return i;
}

int kbd_init(module_t *module) {
  circular_buff_init(&keyboard_buff);
  keyboard_lock = (spinlock_t)SPINLOCK_RELEASED;
  isr_handler_register(KBD_INT, keyboard_isr_handler);
  hal_irq_enable(KBD_IRQ);
  return 0;
}

static const char *kbd_deps[] = {"hal", "devices", NULL};

ITER_MODULE(keyboard) = {
    .name = "keyboard",
    .required = kbd_deps,
    .init = &kbd_init,
    .fini = NULL,
};
