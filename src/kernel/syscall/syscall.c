#include "syscall.h"
#include "device.h"
#include "drivers/keyboard.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/sleep.h"
#include "sched/thread.h"
#include "stdio.h"
#include <stddef.h>
#include <stdint.h>
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

  switch (fd) {
  case 0: // stdin
    device_handle = DEVICE_HANDLE_KEYBOARD;
    break;
  default:
    return -1;
  }

  device_t *dev = get_device_by_handle(device_handle);
  if (!dev)
    return -1;

  // Keyboard has its own read implementation with proper synchronization
  if (device_handle == DEVICE_HANDLE_KEYBOARD) {
    return kbd_read(user_buf, count);
  }

  // Generic read logic for other devices (like disk)
  spinlock_acquire(&dev->wait_queue.lock); // ← ACQUIRE BEFORE CHECK

  while (!dev->data_ready) {
    // Atomically release lock, sleep, and re-acquire lock
    wait_on_queue(&dev->wait_queue, &dev->wait_queue.lock);
  }

  // Now we have the lock AND data is ready
  size_t bytes_to_copy = (count < 256) ? count : 256;
  memcpy(user_buf, dev->buffer, bytes_to_copy);
  dev->data_ready = false;

  spinlock_release(&dev->wait_queue.lock);

  return bytes_to_copy;
}

static long sys_write(int fd, const char *str, size_t len) {
  if (fd == 1 || fd == 2) {
    printf("%s", str);
    return (long)len;
  }
  return -1;
}

void sys_yield(void *context) {
  thread_t *next = sched_pick_next();
  thread_t *current = dispatch_get_current();

  if (current && current != next) {
    sched_enqueue(current); // Re-enqueue current
  }

  dispatch_switch_preserve_context(context, next);
}

void sys_sleep(void *context, uint32_t seconds) {
  thread_t *current = dispatch_get_current();
  uint32_t curr_tick = sched_time_get();
  enqueue_sleeper(current, curr_tick + ((seconds * 1000) / TIMER_TICK_MS));
  thread_t *next = sched_pick_next();
  dispatch_switch_preserve_context(context, next);
}

long sys_fork(void *context) {
  // TODO: Implement fork syscall
  return -1;
}

long sys_execve(const char *pathname, char *const argv[], char *const envp[]) {
  // TODO: Implement execve syscall
  return -1;
}

void sys_exit(int status) {
  // TODO: Implement exit syscall
  while (1)
    ; // Prevent return
}

long sys_waitpid(int pid, int *wstatus, int options) {
  // TODO: Implement waitpid syscall
  return -1;
}

void *sys_sbrk(intptr_t increment) {
  // TODO: Implement sbrk syscall
  return (void *)-1;
}

long sys_open(const char *pathname, int flags) {
  // TODO: Implement open syscall
  return -1;
}

long sys_close(int fd) {
  // TODO: Implement close syscall
  return -1;
}

long syscall_dispatch(syscall_info_t f) {
  switch (f.num) {
  case SYSCALL_NUMBER_SYS_WRITE:
    return sys_write(f.args[0], (const char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_READ:
    return sys_read(f.args[0], (char *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_YIELD:
    sys_yield(f.context);
    return 0;
  case SYSCALL_NUMBER_SYS_SLEEP:
    sys_sleep(f.context, f.args[0]);
    return 0;
  case SYSCALL_NUMBER_SYS_FORK:
    return sys_fork(f.context);
  case SYSCALL_NUMBER_SYS_EXECVE:
    return sys_execve((const char *)f.args[0], (char *const *)f.args[1],
                      (char *const *)f.args[2]);
  case SYSCALL_NUMBER_SYS_EXIT:
    sys_exit(f.args[0]);
    return 0; // Should never reach here
  case SYSCALL_NUMBER_SYS_WAITPID:
    return sys_waitpid(f.args[0], (int *)f.args[1], f.args[2]);
  case SYSCALL_NUMBER_SYS_SBRK:
    return (long)sys_sbrk(f.args[0]);
  case SYSCALL_NUMBER_SYS_OPEN:
    return sys_open((const char *)f.args[0], f.args[1]);
  case SYSCALL_NUMBER_SYS_CLOSE:
    return sys_close(f.args[0]);
  default:
    return -1;
  }
}
