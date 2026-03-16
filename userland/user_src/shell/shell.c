#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 256
#define MAX_ARGS 32

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/* --- Forward declarations --- */
int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);

/* --- Builtin dispatch table --- */
char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&shell_cd, &shell_help, &shell_exit};

static int num_builtins(void) { return sizeof(builtin_str) / sizeof(char *); }

/* --- Line parsing --- */
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

/* --- Builtin: cd --- */
int shell_cd(char **args) {
  if (args[1] == NULL) {
    printf("No args to cd");
  } else {
    if (chdir(args[1]) != 0)
      perror("shell");
  }
  return 1;
}

/* --- Builtin: help --- */
int shell_help(char **args) {
  (void)args;
  printf("Built-in commands:\n");
  for (int i = 0; i < num_builtins(); i++)
    printf("  %s\n", builtin_str[i]);
  return 1;
}

/* --- Builtin: exit --- */
int shell_exit(char **args) {
  (void)args;
  exit(0);
}

/* --- External program launcher --- */
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

/* --- Command dispatcher --- */
int execute_command(char **args) {
  if (args[0] == NULL)
    return 1;

  for (int i = 0; i < num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0)
      return (*builtin_func[i])(args);
  }

  return launch_external_program(args);
}

/* --- Main loop --- */
int main(int argc, char **argv) {
  char line[MAX_LINE];
  char *args[MAX_ARGS];

  printf(ANSI_COLOR_GREEN "\n");

  if (argc > 1) {
    execute_command(&argv[1]);
  }

  printf(ANSI_COLOR_RESET "\n");

  while (1) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET, cwd);
    }
    printf(" $ ");
    fflush(stdout);

    if (fgets(line, sizeof(line), stdin) == NULL)
      break;

    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '\0')
      continue;

    parse_line(line, args);
    execute_command(args);
  }

  return 0;
}
