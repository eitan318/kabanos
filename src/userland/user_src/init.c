#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int proc_spawn(char *name) {
  pid_t pid = fork();

  if (pid < 0) {
    perror("fork failed");
    return 1;
  }

  if (pid == 0) {
    execve(name, NULL, NULL);
    perror("execve failed");
    exit(1);
  }
  return 0;
}

int main(int argc, char **argv, char **envp) {
  proc_spawn("test_a.elf");
  proc_spawn("test_b.elf");
  proc_spawn("test_c.elf");

  for (;;) {
  }

  return 0;
}
