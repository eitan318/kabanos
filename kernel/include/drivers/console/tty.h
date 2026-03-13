#pragma once
#include "sched/wait.h"
#include "spinlock.h"

typedef struct {
  char buf[256];
  int len;
  int read_pos;
  int flags; /* TTY_ICANON | TTY_ECHO */
  spinlock_t lock;
  wait_queue_t wq;
} tty_t;

#define TTY_ICANON (1 << 0) /* cooked: buffer until \n  */
#define TTY_ECHO (1 << 1)   /* echo input back to screen */
