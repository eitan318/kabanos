#include "syscall.h"
#include "hal.h"
#include "isr.h"
#include "stdio.h"
#include <stddef.h>

typedef enum { SYSCALL_NUMBERS_SYS_WRITE = 1 } SYSCALL_NUMBERS;

static long sys_write(const char *str, size_t len) {
  printf("%s", str);
  return 1;
}

long syscall_dispatch(const syscall_frame_t *f) {
  switch (f->num) {
  case SYSCALL_NUMBERS_SYS_WRITE:
    return sys_write((const char *)f->args[0], f->args[1]);
  default:
    return -1;
  }
}
