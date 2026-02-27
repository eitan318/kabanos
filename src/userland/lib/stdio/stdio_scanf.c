// userland
#include "math.h"
#include "stdint.h"
#include "stdio.h"
#include "stdio_internal.h"
#include "unistd.h"

typedef enum {
  LENGTH_DEFAULT,
  LENGTH_SHORT,
  LENGTH_SHORT_SHORT,
  LENGTH_LONG,
  LENGTH_LONG_LONG
} PfLen;
typedef enum {
  STATE_NORMAL,
  STATE_LEN,
  STATE_LEN_LONG,
  STATE_LEN_SHORT,
  STATE_SPEC
} PfState;

static int format_core(FILE *out, const char *fmt, va_list args) {
  PfLen length = LENGTH_DEFAULT;
  PfState state = STATE_NORMAL;
  int written = 0;

  for (; *fmt; fmt++) {
    switch (state) {

    case STATE_NORMAL:
      if (*fmt == '%') {
        state = STATE_LEN;
        length = LENGTH_DEFAULT;
      } else {
        out_putc(out, *fmt);
        written++;
      }
      break;

    case STATE_LEN:
      if (*fmt == 'l') {
        length = LENGTH_LONG;
        state = STATE_LEN_LONG;
      } else if (*fmt == 'h') {
        length = LENGTH_SHORT;
        state = STATE_LEN_SHORT;
      } else {
        state = STATE_SPEC;
        fmt--;
      }
      break;

    case STATE_LEN_LONG:
      if (*fmt == 'l')
        length = LENGTH_LONG_LONG;
      else
        fmt--;
      state = STATE_SPEC;
      break;

    case STATE_LEN_SHORT:
      if (*fmt == 'h')
        length = LENGTH_SHORT_SHORT;
      else
        fmt--;
      state = STATE_SPEC;
      break;

    case STATE_SPEC: {
      state = STATE_NORMAL;
      int radix = 0;
      bool sign = false;
      bool is_num = false;

      switch (*fmt) {
      case 'x':
      case 'X':
      case 'p':
        radix = 16;
        sign = false;
        is_num = true;
        break;
      case 'd':
      case 'i':
        radix = 10;
        sign = true;
        is_num = true;
        break;
      case 'u':
        radix = 10;
        sign = false;
        is_num = true;
        break;
      case 'o':
        radix = 8;
        sign = false;
        is_num = true;
        break;
      case 's':
        out_puts(out, va_arg(args, const char *));
        break;
      case 'c':
        out_putc(out, (char)va_arg(args, int));
        written++;
        break;
      case '%':
        out_putc(out, '%');
        written++;
        break;
      default:
        out_putc(out, '%');
        out_putc(out, *fmt);
        written += 2;
        break;
      }

      if (is_num) {
        if (sign) {
          switch (length) {
          case LENGTH_SHORT_SHORT:
          case LENGTH_SHORT:
          case LENGTH_DEFAULT:
            out_signed(out, va_arg(args, int), radix);
            break;
          case LENGTH_LONG:
            out_signed(out, va_arg(args, long), radix);
            break;
          case LENGTH_LONG_LONG:
            out_signed(out, va_arg(args, long long), radix);
            break;
          }
        } else {
          switch (length) {
          case LENGTH_SHORT_SHORT:
          case LENGTH_SHORT:
          case LENGTH_DEFAULT:
            out_unsigned(out, va_arg(args, unsigned int), radix);
            break;
          case LENGTH_LONG:
            out_unsigned(out, va_arg(args, unsigned long), radix);
            break;
          case LENGTH_LONG_LONG:
            out_unsigned(out, va_arg(args, unsigned long long), radix);
            break;
          }
        }
      }
      break;
    }
    }
  }

  return written;
}
