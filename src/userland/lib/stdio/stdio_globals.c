#include "stdio_internal.h"
#include "unistd.h"

static char stdout_buf[BUFSIZ];
static struct _IO_FILE _stdin_file = {
    .fd = FD_STDIN, .buf = NULL, .buf_size = 0, .buf_pos = 0};
static struct _IO_FILE _stdout_file = {
    .fd = FD_STDOUT, .buf = stdout_buf, .buf_size = BUFSIZ, .buf_pos = 0};
static struct _IO_FILE _stderr_file = {
    .fd = FD_STDERR, .buf = NULL, .buf_size = 0, .buf_pos = 0};

FILE *_open_streams_head;

FILE *stdin = &_stdin_file;
FILE *stdout = &_stdout_file;
FILE *stderr = &_stderr_file;

void add_file(FILE *file) {
  // here is problematic
  file->next = _open_streams_head;
  _open_streams_head = file;
}

void stdio_init() {
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  add_file(stdin);
  add_file(stdout);
  add_file(stderr);
}
