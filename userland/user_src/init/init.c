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
  printf("argc: %d %s %s", argc, argv[0], argv[1]);
  //
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");
  asm volatile("nop");

  proc_spawn("/bin/shell.elf", NULL, NULL);
  for (;;) {
  }
  int status = 0;
  wait(&status);

  printf("Bye Bye");
}
