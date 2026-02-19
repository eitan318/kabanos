#include "posix.h"
#include "stdio.h"
#include <stddef.h>
#include <stdio.h>

int main(void) {
  printf("forking\n");
  pid_t pid = fork();
  printf("after fork\n");

  if (pid == 0) {
    //  This is the CHILD process
    // execve("test_b.elf", NULL, NULL);
    //  If execve fails, we must exit so the child doesn't
    //  keep running the parent's loop.
    //_exit(1);
  }

  execve("test_c.elf", NULL, NULL);
  return 0;
}
