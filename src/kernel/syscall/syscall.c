#include "syscall.h"
#include "device.h"
#include "sched/dispatcher.h"
#include "sched/sched.h"
#include "sched/thread.h"
#include "stdio.h"
#include <stddef.h>

typedef enum {
  SYSCALL_NUMBERS_SYS_WRITE = 1,
  SYSCALL_NUMBERS_SYS_YIELD = 2,   // Added for FCFS rotation
  SYSCALL_NUMBERS_SYS_SLEEP = 3,   // Added for Blocking I/O demo
  SYSCALL_NUMBERS_SYS_HARDWARE = 4 // Added for Blocking I/O demo
} SYSCALL_NUMBERS;

static long sys_write(const char *str, size_t len) {
  printf("%s", str);
  return (long)len;
}

long sys_read(int device_handle, char *user_buf) {
  device_t *dev = get_device_by_handle(device_handle);

  if (!dev->data_ready) {
    // We tell the thread to wait on THIS device's queue
    wait_on_queue(&dev->wait_queue);
  }

  // Pull data from device buffer once woken up
  return copy_to_user(user_buf, dev->buffer);
}

static void handle_yield(void) {
  thread_t *current = dispatch_get_current();

  if (current->tid != 0) { // Don't enqueue the idle task
    sched_enqueue(current);
  }
  thread_t *next = sched_pick_next();
  dispatch_switch_to(next);
}

void block_current_thread() {
  thread_t *current = dispatch_get_current();
  current->state = THREAD_BLOCKED;

  // We do NOT put 'current' back into the ready_queue.
  // It stays in a wait_queue for the hardware.

  thread_t *next = sched_pick_next(); // Get the FCFS head

  // This is the "Point of No Return"
  dispatch_switch_to(next);
}

void hardware_start_async_operation() {}

// This runs when the hardware (Disk/Keyboard/Timer) finishes
void hardware_interrupt_handler() {
  // 1. Find the thread that was waiting for this specific hardware
  thread_t *waiting_thread = get_thread_from_wait_queue();

  // 2. Mark it as READY and put it back in the FCFS queue
  waiting_thread->state = THREAD_READY;
  sched_enqueue(waiting_thread);

  // Note: We DON'T switch immediately here usually.
  // We just put it in line. The next time the current thread
  // yields or its timer expires, sched_pick_next() will see this thread.
}

long syscall_dispatch(const syscall_frame_t *f) {
  switch (f->num) {
  case SYSCALL_NUMBERS_SYS_HARDWARE:
    // 1. Tell the hardware to start working (e.g., Disk Read)
    hardware_start_async_operation();

    // 2. Block until the hardware is done
    block_current_thread();

    /* --- THE GAP ---
       Execution STOPS here. The CPU is now running other threads.
       Weeks, seconds, or milliseconds pass...
       Then, an Interrupt happens, calls thread_wakeup(),
       and eventually sched_pick_next() picks this thread again.
       --- THE RESUMPTION --- */

    // 3. When we reach this line, we have been "Woken up"!
    // The hardware is finished, and we can now collect the result.
    int hardware_result = 8;
    return 8;

  case SYSCALL_NUMBERS_SYS_WRITE:
    return sys_write((const char *)f->args[0], f->args[1]);

  case SYSCALL_NUMBERS_SYS_YIELD:
    handle_yield();
    return 0;

  default:
    return -1;
  }
}
