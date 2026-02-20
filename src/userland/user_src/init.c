#include "posix.h"
#include "stdio.h"
#include <stddef.h>
#include <stdio.h>

int main(void) {
  pid_t pid = fork();
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  printf("pid: %d\n", pid);

  if (pid == 0) {
    //  This is the CHILD process
    execve("test_c.elf", NULL, NULL);
    //  If execve fails, we must exit so the child doesn't
    //  keep running the parent's loop.
    _exit(1);
  }

  printf("forked proc didnt spawn\n");
  yield();
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  return 5;
}
