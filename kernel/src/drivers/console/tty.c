#include "drivers/console/tty.h"
#include "device.h"
#include "drivers/console/console.h"
#include "klib/errno.h"
#include "ksys/types.h"
#include "mm/uaccess.h"
#include <stdint.h>

tty_t g_tty;

tty_t *tty_init(termios_t conf) {
  circular_buff_init(&g_tty.queue);
  g_tty.conf = conf;
  g_tty.lock = (spinlock_t)SPINLOCK_RELEASED;
  return &g_tty;
}

int tty_ioctl(device_t *dev, uint32_t request, void *arg) {
  tty_t *tty = (tty_t *)dev->priv;

  switch (request) {
  case TCGETS:
    if (uaccess_copy_to_user(arg, &tty->conf, sizeof(termios_t)))
      return -EFAULT;
    break;

  case TCSETS:
    if (uaccess_copy_from_user(&tty->conf, arg, sizeof(termios_t)))
      return -EFAULT;

    if (!(tty->conf.c_lflag & TTY_ICANON)) {
      wake_up_queue(&dev->waitq);
    }
    break;

  default:
    return -EINVAL;
  }
  return 0;
}

void tty_input(device_t *dev, char key_ascii) {
  tty_t *tty = &g_tty;

  spinlock_acquire(&tty->lock);

  if (tty->conf.c_lflag & TTY_ICANON) {
    if (key_ascii == '\b' || key_ascii == 127) {
      if (circular_buff_dequeue_last(&tty->queue)) {
        con_backspace();
      }
    } else {
      con_putc(key_ascii);
      circular_buff_enqueue(&tty->queue, (void *)(uintptr_t)key_ascii);

      if (key_ascii == '\n') {
        wake_up_queue(&dev->waitq);
      }
    }
  } else {
    circular_buff_enqueue(&tty->queue, (void *)(uintptr_t)key_ascii);
    wake_up_queue(&dev->waitq);
  }

  spinlock_release(&tty->lock);
}

ssize_t tty_read(device_t *dev, void *buf, size_t size) {
  tty_t *tty = &g_tty;
  size_t bytes_read = 0;
  char *out = (char *)buf;

  while (bytes_read < size) {
    spinlock_acquire(&tty->lock);

    // Sleep if empty
    while (circular_buff_is_empty(&tty->queue)) {
      wait_on_queue(&dev->waitq, &tty->lock);
    }

    // Get the character
    char c = (char)(uintptr_t)circular_buff_dequeue(&tty->queue);
    spinlock_release(&tty->lock);

    out[bytes_read++] = c;

    // In ICANON mode, we return after a newline
    if ((tty->conf.c_lflag & TTY_ICANON) && c == '\n') {
      break;
    }
  }
  return (ssize_t)bytes_read;
}
