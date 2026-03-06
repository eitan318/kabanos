#include "user.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_ARGS 32
#define PROMPT "myos> "

/* --- line parsing --- */
static int parse_line(char *line, char **argv) {
  int argc = 0;
  char *p = line;
  while (*p) {
    while (*p == ' ' || *p == '\t')
      p++;
    if (!*p)
      break;
    argv[argc++] = p;
    if (argc >= MAX_ARGS - 1)
      break;
    while (*p && *p != ' ' && *p != '\t')
      p++;
    if (*p)
      *p++ = '\0';
  }
  argv[argc] = NULL;
  return argc;
}

/* --- builtins --- */
static int cmd_help(int argc, char **argv) {
  puts("usage: <command> [args]");
  puts("  runs /bin/<command>.elf [args]");
  puts("  built-ins: help, exit, clear");
  return 0;
}

/* --- execute --- */
static int execute(int argc, char **argv) {
  if (argc == 0)
    return 0;
  if (strcmp(argv[0], "exit") == 0)
    exit(0);
  if (strcmp(argv[0], "help") == 0)
    return cmd_help(argc, argv);

  char path[128];
  if (argv[0][0] == '/')
    snprintf(path, sizeof(path), "%s", argv[0]);
  else
    snprintf(path, sizeof(path), "/bin/%s.elf", argv[0]);

  // FLUSH before fork — ensures parent output is committed
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

int main(int argc, char **argv, char **envp) {
  char line[MAX_LINE];
  char *args[MAX_ARGS];

  puts("myos shell - type 'help'");

  while (1) {
    printf(PROMPT);
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL)
      break;

    /* strip trailing newline */
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0')
      continue;

    int n = parse_line(line, args);
    execute(n, args);
  }

  return 0;
}
