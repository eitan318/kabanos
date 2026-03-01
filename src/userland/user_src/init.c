#include "user.h"
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int proc_spawn(char *name, char **argv, char **envp) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  }

  if (pid == 0) {
    execve(name, argv, envp);
    perror("execve failed");
    exit(1);
  }
  return 0;
}

int main(int argc, char **argv, char **envp) {
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  zwrite(STDOUT_FILENO, "hello", 5);
  // proc_spawn("/boot/test_a.elf", NULL, NULL);
  // proc_spawn("/boot/test_b.elf", NULL, NULL);
  // proc_spawn("/boot/test_c.elf", NULL, NULL);
  while (1) {
  }
}
