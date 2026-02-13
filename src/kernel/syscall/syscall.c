#include "syscall.h"
#include "device.h"
#include "drivers/keyboard.h"
#include "proc/yield.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "stdio.h"
#include <stddef.h>
#include <string.h>

typedef enum {
  /* --- File & Device I/O --- */
  SYSCALL_NUMBER_SYS_WRITE = 1,
  SYSCALL_NUMBER_SYS_READ = 4,
  SYSCALL_NUMBER_SYS_OPEN = 5,
  SYSCALL_NUMBER_SYS_CLOSE = 6,
  SYSCALL_NUMBER_SYS_LSEEK = 7,
  SYSCALL_NUMBER_SYS_STAT = 8,

  /* --- Process Management --- */
  SYSCALL_NUMBER_SYS_FORK = 10,
  SYSCALL_NUMBER_SYS_EXECVE = 11,
  SYSCALL_NUMBER_SYS_EXIT = 12,
  SYSCALL_NUMBER_SYS_WAITPID = 13,
  SYSCALL_NUMBER_SYS_GETPID = 14,

  /* --- Scheduling & Time --- */
  SYSCALL_NUMBER_SYS_YIELD = 2,
  SYSCALL_NUMBER_SYS_SLEEP = 3,
  SYSCALL_NUMBER_SYS_NANOSLEEP = 15,
  SYSCALL_NUMBER_SYS_GETTIMEOFDAY = 16,

  /* --- Memory Management --- */
  SYSCALL_NUMBER_SYS_SBRK = 20, // For malloc/heap expansion
  SYSCALL_NUMBER_SYS_MMAP = 21,
  SYSCALL_NUMBER_SYS_MUNMAP = 22,

  /* --- Signals & IPC --- */
  SYSCALL_NUMBER_SYS_KILL = 30,
  SYSCALL_NUMBER_SYS_PIPE = 31,
  SYSCALL_NUMBER_SYS_SIGACTION = 32,
} SYSCALL_NUMBER;

typedef enum {
  FD_STDIN = 0,
  FD_STDOUT = 1,
  FD_STDERR = 2,
} STREAM_FD;

static long sys_read(int fd, char *user_buf, size_t count) {
  int device_handle;

  // Map POSIX File Descriptors to Kernel Device Handles
  switch (fd) {
  case 0: // stdin
    device_handle = DEVICE_HANDLE_KEYBOARD;
    break;
  default:
    // For now, return error for other fds
    return -1;
  }

  device_t *dev = get_device_by_handle(device_handle);
  if (!dev)
    return -1;

  if (device_handle == DEVICE_HANDLE_KEYBOARD) {
    size_t bytes_read = 0;
    while (bytes_read < count) {
      // kbd_char_get() will block the thread if the queue is empty
      user_buf[bytes_read] = kbd_char_get();
      bytes_read++;

      // Optional: stop if user hits enter
      if (user_buf[bytes_read - 1] == '\n')
        break;
    }
    return bytes_read;
  }

  // Generic read logic for other devices (like disk)
  while (!dev->data_ready) {
    wait_on_queue(&dev->wait_queue);
  }

  spinlock_acquire(&dev->wait_queue.lock);
  memcpy(user_buf, dev->buffer, (count < 256) ? count : 256);
  dev->data_ready = false;
  spinlock_release(&dev->wait_queue.lock);

  return count;
}

static long sys_write(int fd, const char *str, size_t len) {
  if (fd == 1 || fd == 2) {
    printf("%s", str);
    return (long)len;
  }
  return -1;
}
void example_check(syscall_info_t f) {}

long syscall_dispatch(syscall_info_t f) {

  switch (f.num) {
  case SYSCALL_NUMBER_SYS_WRITE:
    return sys_write(f.args[0], (const char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_READ:
    return sys_read(f.args[0], (char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_YIELD:
    handle_yield(f.context);
  case 67:
    example_check(f);
    return 0;

  default:
    return -1;
  }
}
