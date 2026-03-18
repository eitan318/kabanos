#include "stddef.h"
#include "stdio.h"
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static int launch_external_program(char **argv) {
  char path[128];
  if (argv[0][0] == '/')
    snprintf(path, sizeof(path), "%s", argv[0]);
  else
    snprintf(path, sizeof(path), "/bin/%s.elf", argv[0]);

  fflush(stdout);
  fflush(stderr);

  int pid = fork();
  if (pid < 0) {
    fprintf(stderr, "shell: fork failed\n");
    fflush(stderr);
    return -1;
  }
  if (pid == 0) {
    execve(path, argv, NULL);
    fprintf(stderr, "shell: %s: not found\n", argv[0]);
    fflush(stderr);
    exit(1);
  }

  int status = 0;
  wait(&status);
  return status;
}

/* --- ANSI Terminal Control Definitions --- */
#define ANSI_CLS "\x1b[2J"            // Clear screen
#define ANSI_HOME "\x1b[H"            // Move cursor to 1,1
#define ANSI_GOTO_PROMPT "\x1b[15;1H" // Move to line 15 (start of shell)

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int main() {
  char *cmd1[] = {"cat", "/header_icon.txt", NULL};
  char *cmd2[] = {"cat", "/header_title.txt", NULL};

  // 1. Wipe the screen and start at the top
  printf(ANSI_CLS ANSI_HOME);
  fflush(stdout);

  // 2. Render the Yellow Sausage Icon
  printf(ANSI_COLOR_YELLOW);
  fflush(stdout);
  launch_external_program(cmd1);

  // 3. Reset to default theme colors (Light/Dark mode)
  printf(ANSI_COLOR_RESET);
  fflush(stdout);

  // 4. Move back to top to overlay/align the Title
  printf(ANSI_HOME);
  fflush(stdout);
  launch_external_program(cmd2);

  // 5. Jump past the logo to prevent the shell prompt from overwriting it
  printf(ANSI_GOTO_PROMPT "\n");
  fflush(stdout);

  return 0;
}
