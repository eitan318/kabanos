#pragma once
#include "adt/circular_buffer.h"
#include "device.h"
#include "ksys/types.h"
#include "sched/wait.h"
#include "spinlock.h"

typedef struct {
  circular_buff_t queue;
  int flags; /* TTY_ICANON | TTY_ECHO */
  spinlock_t lock;
  // wait_queue_t wq;
} tty_t;

#define TTY_ICANON (1 << 0) /* cooked: buffer until \n  */
#define TTY_ECHO (1 << 1)   /* echo input back to screen */

#define TCGETS 0x5401
#define TCSETS 0x5402

tty_t *tty_init(uint32_t flags);
void tty_input(device_t *dev, char key_ascii);
ssize_t tty_read(device_t *dev, void *buf, size_t size);
int tty_ioctl(device_t *dev, uint32_t request, void *arg);
