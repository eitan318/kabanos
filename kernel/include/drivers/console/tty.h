#pragma once
#include "adt/circular_buffer.h"
#include "device.h"
#include "ksys/types.h"
#include "sched/wait.h"
#include "spinlock.h"

#define NCCS 32
typedef uint32_t tcflag_t;
typedef uint8_t cc_t;
typedef struct {
  tcflag_t c_iflag; /* input modes */
  tcflag_t c_oflag; /* output modes */
  tcflag_t c_cflag; /* control modes */
  tcflag_t c_lflag; /* local modes */
  cc_t c_cc[NCCS];  /* special characters */
} termios_t;

typedef struct {
  termios_t conf;
  circular_buff_t queue;
  spinlock_t lock;
} tty_t;

#define TTY_ICANON (1 << 0) /* cooked: buffer until \n  */
#define TTY_ECHO (1 << 1)   /* echo input back to screen */

#define TCGETS 0x5401
#define TCSETS 0x5402

tty_t *tty_init(termios_t conf);
void tty_input(device_t *dev, char key_ascii);
ssize_t tty_read(device_t *dev, void *buf, size_t size);
int tty_ioctl(device_t *dev, uint32_t request, void *arg);
