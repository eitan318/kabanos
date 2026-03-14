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
  char *shell_args[] = {"/bin/shell.elf", "/bin/cat.elf", "header.txt", NULL};

  // Spawn the shell
  if (proc_spawn("/bin/shell.elf", shell_args, envp) < 0) {
    printf("Init: Failed to start shell!\n");
    return 1;
  }

  /// There is a schedualer bug preventing from one process to switch to itself
  /// so there allways gotta be at least one proc
  for (;;) {
  }

  while (1) {
    int status;
    if (wait(&status) > 0) {
      printf("Init: Shell exited. Restarting...\n");
      proc_spawn("/bin/shell.elf", shell_args, envp);
    }
  }
}
