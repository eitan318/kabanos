#include "math.h"
#include "stdint.h"
#include "stdio.h"
#include "stdio_internal.h"
#include "unistd.h"

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
