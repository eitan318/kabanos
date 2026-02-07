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
  SYSCALL_NUMBERS_SYS_WRITE = 1,
  SYSCALL_NUMBERS_SYS_YIELD = 2, // Added for FCFS rotation
  SYSCALL_NUMBERS_SYS_SLEEP = 3, // Added for Blocking I/O demo
  SYSCALL_NUMBERS_SYS_READ = 4   // Added for Blocking I/O demo
} SYSCALL_NUMBERS;

static long sys_write(const char *str, size_t len) {
  printf("%s", str);
  return (long)len;
}

long sys_read(int device_handle, char *user_buf, size_t count) {
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

long syscall_dispatch(const syscall_frame_t *f) {
  switch (f->num) {
  case SYSCALL_NUMBERS_SYS_WRITE:
    return sys_write((const char *)f->args[0], f->args[1]);
  case SYSCALL_NUMBERS_SYS_READ:
    return sys_read(f->args[0], (char *)f->args[1], f->args[2]);
  case SYSCALL_NUMBERS_SYS_YIELD:
    handle_yield();
    return 0;

  default:
    return -1;
  }
}
