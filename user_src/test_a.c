#include "include/syscall.h"
#include <stddef.h>

void _start(void) {
  char key_buf[1];
  char *msg = "Process A waiting for key...\n";

  while (1) {
    _syscall6(SYSCALL_NUMBERS_SYS_WRITE, msg, 29, 0, 0, 0, 0);

    // This is the trigger. Process A will BLOCK here.
    _syscall6(SYSCALL_NUMBERS_SYS_READ, DEVICE_HANDLE_KEYBOARD, key_buf, 1, 0,
              0, 0);

    // This will ONLY print AFTER you press a key.
    char *done = "Process A woke up!\n";
    _syscall6(SYSCALL_NUMBERS_SYS_WRITE, done, 19, 0, 0, 0, 0);
  }
  for (;;)
    ;
}
