// stage2 stdio.c
#include "utils/math.h"
#include "vfs.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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
  uint8_t byte = (uint8_t)c; // Convert to unsigned first
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
  // Only write if there's space (reserve 1 byte for null terminator)
  if (buf_ctx->pos < buf_ctx->size - 1) {
    buf_ctx->buffer[buf_ctx->pos] = c;
  }
  buf_ctx->pos++; // Always increment to track total chars that would be written
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
      // Add a breakpoint or error message here
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

// Core printf implementation - works with any output
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
        // chars_written += strlen(str); // Could track this if needed
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
// Public API - each variant sets up its output context
// ============================================================================

void vfprintf(fd_t file, const char *format, va_list args) {
  fd_output_ctx_t ctx = {.file = file};
  printf_output_t out = {.putc_fn = fd_putc, .context = &ctx};
  vprintf_core(&out, format, args);
}

int vsnprintf(char *buffer, size_t size, const char *format, va_list args) {
  if (size == 0)
    return 0;

  buffer_output_ctx_t ctx = {.buffer = buffer, .size = size, .pos = 0};
  printf_output_t out = {.putc_fn = buffer_putc, .context = &ctx};

  vprintf_core(&out, format, args);

  // Null-terminate the buffer
  if (ctx.pos < size) {
    buffer[ctx.pos] = '\0';
  } else {
    buffer[size - 1] = '\0';
  }

  return ctx.pos; // Return number of chars that would have been written
}

int vsprintf(char *buffer, const char *format, va_list args) {
  // Unsafe version - no size limit
  return vsnprintf(buffer, SIZE_MAX, format, args);
}

// ============================================================================
// Convenience wrappers
// ============================================================================

void fprintf(fd_t file, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(file, fmt, args);
  va_end(args);
}

void printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(VFS_FD_STDOUT, fmt, args);
  va_end(args);
}

void debugf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(VFS_FD_DEBUG, fmt, args);
  va_end(args);
}

void debugf_and_printf(const char *fmt, ...) {
  va_list args, args_copy;

  va_start(args, fmt);
  va_copy(args_copy, args);
  vfprintf(VFS_FD_DEBUG, fmt, args_copy);
  vfprintf(VFS_FD_STDOUT, fmt, args);
  va_end(args_copy);
  va_end(args);
}

int snprintf(char *buffer, size_t size, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(buffer, size, fmt, args);
  va_end(args);
  return ret;
}

int sprintf(char *buffer, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(buffer, fmt, args);
  va_end(args);
  return ret;
}

// ============================================================================
// Simple character/string functions
// ============================================================================

void fputc(char c, fd_t file) { pvfs_write(file, (uint8_t *)&c, sizeof(c)); }

void fputs(const char *s, fd_t file) {
  while (*s) {
    fputc(*s, file);
    s++;
  }
}

void putc(char c) { fputc(c, VFS_FD_STDOUT); }

void puts(const char *s) { fputs(s, VFS_FD_STDOUT); }

void debugc(char c) { fputc(c, VFS_FD_DEBUG); }

void debugs(const char *s) { fputs(s, VFS_FD_DEBUG); }
