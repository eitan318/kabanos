// STDIO for custom libc
#include "stdio.h"
#include "math.h"
#include "posix.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// FILE structure and globals
// ============================================================================
typedef struct _IO_FILE {
  int fd;
  char *buf;
  size_t buf_size;
  size_t buf_pos;
} FILE;

#define BUFSIZ 0x10000

static char stdout_buf[BUFSIZ];
static struct _IO_FILE _stdin_file = {
    .fd = FD_STDIN, .buf = NULL, .buf_size = 0, .buf_pos = 0};
static struct _IO_FILE _stdout_file = {
    .fd = FD_STDOUT, .buf = stdout_buf, .buf_size = BUFSIZ, .buf_pos = 0};
static struct _IO_FILE _stderr_file = {
    .fd = FD_STDERR, .buf = NULL, .buf_size = 0, .buf_pos = 0};

FILE *const stdin = &_stdin_file;
FILE *const stdout = &_stdout_file;
FILE *const stderr = &_stderr_file;

static int file_putc(int c, FILE *file) {
  uint8_t byte = (uint8_t)c;

  if (file->buf && file->buf_size > 0) {
    file->buf[file->buf_pos] = (char)byte;
    // only track pos for snprintf, or flush for real files
    if (file->fd == -1) {
      // buffer-only mode (snprintf): just advance, never flush
      if (file->buf_pos < file->buf_size - 1)
        file->buf_pos++;
    } else {
      file->buf_pos++;
      if (file->buf_pos >= file->buf_size || byte == '\n') {
        write(file->fd, file->buf, file->buf_pos);
        file->buf_pos = 0;
      }
    }
  } else {
    write(file->fd, &byte, 1);
  }

  return (unsigned char)c;
}

int fflush(FILE *file) {
  if (file && file->buf && file->buf_pos > 0) {
    write(file->fd, file->buf, file->buf_pos);
    file->buf_pos = 0;
  }
  return 0;
}
static void out_putc(FILE *out, char c) {
  if (out) {
    file_putc((unsigned char)c, out);
  } else if (out->buf) {
    if (out->buf_pos < out->buf_size - 1)
      out->buf[out->buf_pos] = c;
    out->buf_pos++;
  }
}

static void out_puts(FILE *out, const char *s) {
  while (*s)
    out_putc(out, *s++);
}

// ============================================================================
// Core formatting engine
// ============================================================================

static const char g_hex_chars[] = "0123456789abcdef";

static void out_unsigned(FILE *out, unsigned long long n, int radix) {
  char tmp[32];
  int pos = 0;
  do {
    uint32_t rem;
    uint64_t quot;
    div64_32(n, radix, &quot, &rem);
    tmp[pos++] = g_hex_chars[rem];
    n = quot;
  } while (n > 0);
  for (int i = pos - 1; i >= 0; i--)
    out_putc(out, tmp[i]);
}

static void out_signed(FILE *out, long long n, int radix) {
  if (n < 0) {
    out_putc(out, '-');
    out_unsigned(out, (unsigned long long)-n, radix);
  } else {
    out_unsigned(out, (unsigned long long)n, radix);
  }
}

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

static int vprintf_core(FILE *out, const char *fmt, va_list args) {
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

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args) {
  if (size == 0)
    return 0;
  FILE tmp = {.fd = -1, .buf = buf, .buf_size = size, .buf_pos = 0};
  int n = vprintf_core(&tmp, fmt, args);
  buf[tmp.buf_pos < size ? tmp.buf_pos : size - 1] = '\0';
  return n;
}

int vfprintf(FILE *__restrict file, const char *__restrict fmt, va_list args) {
  return vprintf_core(file, fmt, args);
}

int vprintf(const char *__restrict fmt, va_list args) {
  return vfprintf(stdout, fmt, args);
}

int vsprintf(char *__restrict buf, const char *__restrict fmt, va_list args) {
  return vsnprintf(buf, SIZE_MAX, fmt, args);
}

int fprintf(FILE *__restrict file, const char *__restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vfprintf(file, fmt, args);
  va_end(args);
  return ret;
}

int printf(const char *__restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vfprintf(stdout, fmt, args);
  va_end(args);
  return ret;
}

int snprintf(char *__restrict buf, size_t size, const char *__restrict fmt,
             ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(buf, size, fmt, args);
  va_end(args);
  return ret;
}

int sprintf(char *__restrict buf, const char *__restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(buf, fmt, args);
  va_end(args);
  return ret;
}

int fputc(int c, FILE *file) { return file_putc(c, file); }
int putc(int c, FILE *file) { return file_putc(c, file); }
int putchar(int c) { return file_putc(c, stdout); }

int fputs(const char *__restrict s, FILE *__restrict file) {
  while (*s)
    if (file_putc((unsigned char)*s++, file) == EOF)
      return EOF;
  return 0;
}

int puts(const char *s) {
  if (fputs(s, stdout) == EOF)
    return EOF;
  return file_putc('\n', stdout);
}
