#include "stdio_internal.h"
#include "unistd.h"

static char stdout_buf[BUFSIZ];
static struct _IO_FILE _stdin_file = {
    .fd = STDIN_FILENO, .buf = NULL, .buf_size = 0, .buf_pos = 0};
static struct _IO_FILE _stdout_file = {
    .fd = STDOUT_FILENO, .buf = stdout_buf, .buf_size = BUFSIZ, .buf_pos = 0};
static struct _IO_FILE _stderr_file = {
    .fd = STDERR_FILENO, .buf = NULL, .buf_size = 0, .buf_pos = 0};

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
  add_file(stdin);
  add_file(stdout);
  add_file(stderr);
}
