#include "stdio.h"
#include "utils/math.h"
#include "vfs.h"
#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// FILE* <-> fd_t mapping
// ============================================================================

struct _IO_FILE {
  fd_t fd;
};

static struct _IO_FILE _stdin_file = {.fd = VFS_FD_STDIN};
static struct _IO_FILE _stdout_file = {.fd = VFS_FD_STDOUT};
static struct _IO_FILE _stderr_file = {.fd = VFS_FD_STDERR};

FILE *const stdin = &_stdin_file;
FILE *const stdout = &_stdout_file;
FILE *const stderr = &_stderr_file;

// ============================================================================
// Output abstraction layer
// ============================================================================

typedef struct {
  void (*putc_fn)(void *ctx, char c);
  void *context;
} printf_output_t;

// File descriptor output context
typedef struct {
  fd_t file;
} fd_output_ctx_t;

static void fd_putc(void *ctx, char c) {
  fd_output_ctx_t *fd_ctx = (fd_output_ctx_t *)ctx;
  uint8_t byte = (uint8_t)c;
  pvfs_write(fd_ctx->file, &byte, sizeof(byte));
}

// Buffer output context
typedef struct {
  char *buffer;
  size_t size;
  size_t pos;
} buffer_output_ctx_t;

static void buffer_putc(void *ctx, char c) {
  buffer_output_ctx_t *buf_ctx = (buffer_output_ctx_t *)ctx;
  if (buf_ctx->pos < buf_ctx->size - 1) {
    buf_ctx->buffer[buf_ctx->pos] = c;
  }
  buf_ctx->pos++;
}

// ============================================================================
// Core formatting logic (shared by all printf variants)
// ============================================================================

static void output_char(printf_output_t *out, char c) {
  out->putc_fn(out->context, c);
}

static void output_string(printf_output_t *out, const char *s) {
  while (*s) {
    output_char(out, *s);
    s++;
  }
}

static const char g_hex_chars[] = "0123456789abcdef";

static void output_unsigned(printf_output_t *out, unsigned long long number,
                            int radix) {
  char buffer[32];
  int pos = 0;

  do {
    uint32_t reminder_out;
    uint64_t quotient_out;
    div64_32(number, radix, &quotient_out, &reminder_out);
    number = quotient_out;
    buffer[pos] = g_hex_chars[reminder_out];
    pos++;
  } while (number > 0);
  for (int i = pos - 1; i >= 0; i--) {
    if (pos < 0 || pos >= 32) {
      break;
    }
    output_char(out, buffer[i]);
  }
}

static void output_signed(printf_output_t *out, long long number, int radix) {
  if (number < 0) {
    output_char(out, '-');
    output_unsigned(out, -number, radix);
  } else {
    output_unsigned(out, number, radix);
  }
}

typedef enum {
  PRINTF_LENGTH_DEFAULT,
  PRINTF_LENGTH_SHORT,
  PRINTF_LENGTH_SHORT_SHORT,
  PRINTF_LENGTH_LONG,
  PRINTF_LENGTH_LONG_LONG,
} PrintfLength;

typedef enum {
  PRINTF_STATE_NORMAL,
  PRINTF_STATE_LENGTH,
  PRINTF_STATE_LENGTH_LONG,
  PRINTF_STATE_LENGTH_SHORT,
  PRINTF_STATE_SPECIFIER,
} PrintfState;

static int vprintf_core(printf_output_t *out, const char *format,
                        va_list args) {
  PrintfLength length = PRINTF_LENGTH_DEFAULT;
  PrintfState state = PRINTF_STATE_NORMAL;
  int chars_written = 0;

  for (; *format != '\0'; format++) {
    switch (state) {
    case PRINTF_STATE_NORMAL:
      if (*format == '%') {
        state = PRINTF_STATE_LENGTH;
        length = PRINTF_LENGTH_DEFAULT;
      } else {
        output_char(out, *format);
        chars_written++;
      }
      break;

    case PRINTF_STATE_LENGTH:
      switch (*format) {
      case 'l':
        length = PRINTF_LENGTH_LONG;
        state = PRINTF_STATE_LENGTH_LONG;
        break;
      case 'h':
        length = PRINTF_LENGTH_SHORT;
        state = PRINTF_STATE_LENGTH_SHORT;
        break;
      default:
        state = PRINTF_STATE_SPECIFIER;
        format--;
        break;
      }
      break;

    case PRINTF_STATE_LENGTH_LONG:
      if (*format == 'l') {
        length = PRINTF_LENGTH_LONG_LONG;
      } else {
        format--;
      }
      state = PRINTF_STATE_SPECIFIER;
      break;

    case PRINTF_STATE_LENGTH_SHORT:
      if (*format == 'h') {
        length = PRINTF_LENGTH_SHORT_SHORT;
      } else {
        format--;
      }
      state = PRINTF_STATE_SPECIFIER;
      break;

    case PRINTF_STATE_SPECIFIER:
      state = PRINTF_STATE_NORMAL;
      int radix = 0;
      bool sign = false;
      bool number = false;

      switch (*format) {
      case 'x':
      case 'X':
      case 'p':
        sign = false;
        radix = 16;
        number = true;
        break;
      case 'd':
      case 'i':
        sign = true;
        radix = 10;
        number = true;
        break;
      case 'u':
        sign = false;
        radix = 10;
        number = true;
        break;
      case 'o':
        sign = false;
        radix = 8;
        number = true;
        break;
      case 's': {
        const char *str = va_arg(args, const char *);
        output_string(out, str);
        break;
      }
      case 'c':
        output_char(out, (char)va_arg(args, int));
        chars_written++;
        break;
      case '%':
        output_char(out, '%');
        chars_written++;
        break;
      default:
        output_char(out, '%');
        output_char(out, *format);
        chars_written += 2;
        break;
      }

      if (number) {
        if (sign) {
          switch (length) {
          case PRINTF_LENGTH_SHORT_SHORT:
          case PRINTF_LENGTH_SHORT:
          case PRINTF_LENGTH_DEFAULT:
            output_signed(out, va_arg(args, int), radix);
            break;
          case PRINTF_LENGTH_LONG:
            output_signed(out, va_arg(args, long), radix);
            break;
          case PRINTF_LENGTH_LONG_LONG:
            output_signed(out, va_arg(args, long long), radix);
            break;
          }
        } else {
          switch (length) {
          case PRINTF_LENGTH_SHORT_SHORT:
          case PRINTF_LENGTH_SHORT:
          case PRINTF_LENGTH_DEFAULT:
            output_unsigned(out, va_arg(args, unsigned int), radix);
            break;
          case PRINTF_LENGTH_LONG:
            output_unsigned(out, va_arg(args, unsigned long), radix);
            break;
          case PRINTF_LENGTH_LONG_LONG:
            output_unsigned(out, va_arg(args, unsigned long long), radix);
            break;
          }
        }
      }
      break;
    }
  }

  return chars_written;
}

// ============================================================================
// Internal helpers using fd_t (used by debugf etc.)
// ============================================================================

static void vfprintf_fd(fd_t fd, const char *format, va_list args) {
  fd_output_ctx_t ctx = {.file = fd};
  printf_output_t out = {.putc_fn = fd_putc, .context = &ctx};
  vprintf_core(&out, format, args);
}

// ============================================================================
// Public API - matching stdio.h signatures
// ============================================================================

int vfprintf(FILE *__restrict stream, const char *__restrict format,
             va_list args) {
  fd_output_ctx_t ctx = {.file = stream->fd};
  printf_output_t out = {.putc_fn = fd_putc, .context = &ctx};
  return vprintf_core(&out, format, args);
}

int vprintf(const char *__restrict format, va_list args) {
  return vfprintf(stdout, format, args);
}

int vsnprintf(char *__restrict buffer, size_t size,
              const char *__restrict format, va_list args) {
  if (size == 0)
    return 0;

  buffer_output_ctx_t ctx = {.buffer = buffer, .size = size, .pos = 0};
  printf_output_t out = {.putc_fn = buffer_putc, .context = &ctx};

  vprintf_core(&out, format, args);

  if (ctx.pos < size) {
    buffer[ctx.pos] = '\0';
  } else {
    buffer[size - 1] = '\0';
  }

  return (int)ctx.pos;
}

int vsprintf(char *__restrict buffer, const char *__restrict format,
             va_list args) {
  return vsnprintf(buffer, SIZE_MAX, format, args);
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

int snprintf(char *__restrict buffer, size_t size, const char *__restrict fmt,
             ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(buffer, size, fmt, args);
  va_end(args);
  return ret;
}

int sprintf(char *__restrict buffer, const char *__restrict fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(buffer, fmt, args);
  va_end(args);
  return ret;
}

// ============================================================================
// Simple character/string functions
// ============================================================================

int fputc(int c, FILE *file) {
  uint8_t byte = (uint8_t)c;
  pvfs_write(file->fd, &byte, sizeof(byte));
  return (unsigned char)c;
}

int fputs(const char *__restrict s, FILE *__restrict file) {
  while (*s) {
    fputc((unsigned char)*s, file);
    s++;
  }
  return 0;
}

int putc(int c, FILE *file) { return fputc(c, file); }

int putchar(int c) { return fputc(c, stdout); }

int puts(const char *s) {
  fputs(s, stdout);
  fputc('\n', stdout);
  return 0;
}

// ============================================================================
// Debug variants (internal, not in stdio.h)
// ============================================================================

void debugf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf_fd(VFS_FD_DEBUG, fmt, args);
  va_end(args);
}

void debugf_and_printf(const char *fmt, ...) {
  va_list args, args_copy;

  va_start(args, fmt);
  va_copy(args_copy, args);
  vfprintf_fd(VFS_FD_DEBUG, fmt, args_copy);
  vfprintf(stdout, fmt, args);
  va_end(args_copy);
  va_end(args);
}

void debugc(char c) { fputc((unsigned char)c, stderr); }

void debugs(const char *s) { fputs(s, stderr); }
