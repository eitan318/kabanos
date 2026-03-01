#include "user.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
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
  int num = 0;
  scanf("%d", &num);
  printf("len %d", strlen("el primi"));
  proc_spawn("/boot/test_a.elf", NULL, NULL);
  proc_spawn("/boot/test_b.elf", NULL, NULL);
  proc_spawn("/boot/test_c.elf", NULL, NULL);
  while (1) {
  }
}
