#include "syscall.h"
#include "device.h"
#include "drivers/keyboard.h"
#include "proc/yield.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "stdio.h"
#include "syscall/errno.h"
#include <stddef.h>
#include <string.h>

typedef enum {
  SYSCALL_NUMBERS_SYS_WRITE = 1,
  SYSCALL_NUMBERS_SYS_YIELD = 2, // Added for FCFS rotation
  SYSCALL_NUMBERS_SYS_SLEEP = 3, // Added for Blocking I/O demo
  SYSCALL_NUMBERS_SYS_READ = 4   // Added for Blocking I/O demo
} SYSCALL_NUMBERS;

static long sys_write(const char *str, size_t len) {
  printf("%s", str);
  return (long)len;
}

long sys_read(int fd, char *user_buf, size_t count) {
  // POSIX Mapping
  int device_handle = fd;
  if (fd == 0)
    device_handle = DEVICE_HANDLE_KEYBOARD;

  device_t *dev = get_device_by_handle(device_handle);
  if (!dev)
    return -EBADF; // POSIX Error: Bad File Descriptor

  size_t bytes_read = 0;
  while (bytes_read < count) {
    char c = kbd_char_get();
    user_buf[bytes_read++] = c;

    if (c == '\n')
      break;
  }

  return (long)bytes_read;
}

long syscall_dispatch(syscall_info_t f) {

  switch (f.num) {
  case SYSCALL_NUMBERS_SYS_WRITE:
    return sys_write((const char *)f.args[0], f.args[1]);
  case SYSCALL_NUMBERS_SYS_READ:
    return sys_read(f.args[0], (char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBERS_SYS_YIELD:
    handle_yield(f.context);
    return 0;

  default:
    return -1;
  }
}
