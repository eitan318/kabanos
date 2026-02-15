#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  pid_t pid = fork();

  if (pid == 0) {
    // This is the CHILD process
    execve("test_b.elf", NULL, NULL);
    // If execve fails, we must exit so the child doesn't
    // keep running the parent's loop.
    _exit(1);
  }

  // This is the PARENT process (the original _start)
  // It continues here immediately while test_a runs.
  execve("test_c.elf", NULL, NULL);
}
