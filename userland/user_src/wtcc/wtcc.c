#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: tccw <file.c> [-o output]\n");
    return 1;
  }

  // Allocate enough space for hardcoded flags + passed flags + NULL
  char *new_argv[32];
  int i = 0;

  new_argv[i++] = "/bin/tcc.elf";
  new_argv[i++] = "-nostdlib";
  new_argv[i++] = "-I/usr/include";
  new_argv[i++] = "-I/usr/lib/tcc/include";
  new_argv[i++] = "/usr/lib/crt0.o";

  // Add the source file (e.g., hello.c)
  new_argv[i++] = argv[1];

  // Pass through any remaining flags from the command line (like -o bin)
  for (int j = 2; j < argc && i < 28; j++) {
    new_argv[i++] = argv[j];
  }

  // Append libraries at the end
  new_argv[i++] = "-lc";
  new_argv[i++] = "-lnosys";
  new_argv[i] = NULL;

  pid_t pid = fork();

  if (pid == 0) {
    // Child: Execute the actual compiler
    execve(new_argv[0], new_argv, NULL);

    // If execve returns, it failed
    perror("execve");
    exit(1);
  } else if (pid > 0) {
    // Parent: Wait for TCC to finish
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
  } else {
    perror("fork");
    return 1;
  }
}
