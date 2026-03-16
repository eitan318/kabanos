#ifndef _TERMIOS_H_
#define _TERMIOS_H_

#include <stdint.h>

typedef uint32_t tcflag_t;
typedef uint8_t cc_t;

#define NCCS 32

struct termios {
  tcflag_t c_iflag; /* input modes */
  tcflag_t c_oflag; /* output modes */
  tcflag_t c_cflag; /* control modes */
  tcflag_t c_lflag; /* local modes */
  cc_t c_cc[NCCS];  /* special characters */
};

/* c_iflag bits */
#define BRKINT 0x0002
#define ICRNL 0x0100
#define INPCK 0x0010
#define ISTRIP 0x0020
#define IXON 0x0400

/* c_oflag bits */
#define OPOST 0x0001

/* c_cflag bits */
#define CS8 0x0030

/* c_lflag bits - THE IMPORTANT ONES */
#define ISIG 0x0001
#define ICANON 0x0002
#define ECHO 0x0008
#define IEXTEN 0x8000

/* c_cc indices */
#define VMIN 6
#define VTIME 5

/* Commands for tcsetattr/tcgetattr ioctls */
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TCSAFLUSH TCSETS // You can simplify and treat all TCSETS the same

#endif
