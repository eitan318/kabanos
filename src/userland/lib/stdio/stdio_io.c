#include "math.h"
#include "stdint.h"
#include "stdio.h"
#include "stdio_internal.h"
#include "unistd.h"

static int file_putc(int c, FILE *file) {
  uint8_t byte = (uint8_t)c;

  if (file->buf && file->buf_size > 0) {

    if (file->fd == -1) {
      if (file->buf_pos < file->buf_size - 1) {
        file->buf[file->buf_pos++] = (char)byte;
        file->buf[file->buf_pos] = '\0'; // Always null-terminate strings
      }
      return (unsigned char)c;
    }

    file->buf[file->buf_pos++] = (char)byte;

    if (file->buf_pos >= file->buf_size || byte == '\n') {
      write(file->fd, file->buf, file->buf_pos);
      file->buf_pos = 0;
    }
  } else {
    write(file->fd, &byte, FD_STDOUT);
  }

  return (unsigned char)c;
}

// NULL is flush all
int fflush(FILE *stream) {
  if (stream == NULL) {
    for (FILE *curr = _open_streams_head; curr != NULL; curr = curr->next) {
      fflush(curr);
    }
    return 0;
  }

  if (stream->fd != -1 && stream->buf_pos > 0) {
    write(stream->fd, stream->buf, stream->buf_pos);
    stream->buf_pos = 0;
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
