/**
 * @file tty.h
 * @brief TTY line discipline: input buffering, canonical mode and echo.
 */
#pragma once
#include "adt/circular_buffer.h"
#include "device.h"
#include "ksys/types.h"
#include "sched/wait.h"
#include "spinlock.h"

#define NCCS 32
typedef uint32_t tcflag_t;
typedef uint8_t cc_t;

/** @brief Terminal configuration (subset of POSIX termios). */
typedef struct {
  tcflag_t c_iflag; /**< Input modes. */
  tcflag_t c_oflag; /**< Output modes. */
  tcflag_t c_cflag; /**< Control modes. */
  tcflag_t c_lflag; /**< Local modes (TTY_ICANON, TTY_ECHO). */
  cc_t c_cc[NCCS];  /**< Special characters. */
} termios_t;

/** @brief TTY instance state. */
typedef struct {
  termios_t conf;
  circular_buff_t queue; /**< Pending input characters. */
  spinlock_t lock;
} tty_t;

/* c_lflag bits */
#define TTY_ICANON (1 << 0) /**< Cooked mode: buffer input until '\n'. */
#define TTY_ECHO (1 << 1)   /**< Echo input back to the screen. */

/* ioctl requests */
#define TCGETS 0x5401 /**< Get termios configuration. */
#define TCSETS 0x5402 /**< Set termios configuration. */

/** @brief Allocates a TTY with the given initial configuration. */
tty_t *tty_init(termios_t conf);

/**
 * @brief Feeds one input character from the keyboard driver into the TTY,
 *        applying echo and canonical-mode editing.
 */
void tty_input(device_t *dev, char key_ascii);

/**
 * @brief Reads buffered input; blocks until data is available (a full line
 *        in canonical mode).
 */
ssize_t tty_read(device_t *dev, void *buf, size_t size);

/** @brief Handles TCGETS/TCSETS requests. */
int tty_ioctl(device_t *dev, uint32_t request, void *arg);
