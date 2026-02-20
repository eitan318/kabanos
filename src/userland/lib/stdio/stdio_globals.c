#include "stdio_internal.h"
#include "unistd.h"

static char stdout_buf[BUFSIZ];
static struct _IO_FILE _stdin_file = {
    .fd = FD_STDIN, .buf = NULL, .buf_size = 0, .buf_pos = 0};
static struct _IO_FILE _stdout_file = {
    .fd = FD_STDOUT, .buf = stdout_buf, .buf_size = BUFSIZ, .buf_pos = 0};
static struct _IO_FILE _stderr_file = {
    .fd = FD_STDERR, .buf = NULL, .buf_size = 0, .buf_pos = 0};

FILE *stdin = &_stdin_file;
FILE *stdout = &_stdout_file;
FILE *stderr = &_stderr_file;
