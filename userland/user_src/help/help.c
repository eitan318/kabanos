#include "stdio.h"
#include "string.h"
#include <dirent.h>

struct CommandHelp {
  char *name;    // The command (e.g., "ls")
  char *summary; // Brief one-liner
  char *details; // Longer explanation (the 'man' part)
};

// clang-format off
struct CommandHelp help_table[] = {
    {   
        "help",
        "Display information about built-in commands.",
        "Usage: help [-l] [command]"
    },
    {
        "ls",
        "List directory contents.",
        "Usage: ls [directory]\nOptions: -a (all), -l (long format)"
    },
    {
        "cd",
        "Change the working directory.",
        "Usage: cd [path]\nUse '..' to go up one level."
    },
    {
        "pwd",
        "Print the current working directory.",
        "Usage: pwd"
    },
    {
        "cat",
        "Display file contents.",
        "Usage: cat [file]"
    },
    {
        "echo",
        "Print text to the terminal.",
        "Usage: echo [text]"
    },
    {
        "mkdir",
        "Create a new directory.",
        "Usage: mkdir [directory]"
    },
    {
        "touch",
        "Create an empty file.",
        "Usage: touch [file]"
    },
    {
        "rm",
        "Remove a file.",
        "Usage: rm [file]"
    },
    {
        "mv",
        "Move a file from source to destination.",
        "Usage: mv [src] [dst]"
    },
    {
        "cp",
        "Copy a file from source to destination.",
        "Usage: mv [src] [dst]"
    },
    {
        "rmdir",
        "Remove an empty directory.",
        "Usage: rmdir [directory]"
    },
    {
        "clear",
        "Clear the terminal screen.",
        "Usage: clear"
    },
    {
        "ping",
        "pinging to an ip to check if its up.",
        "Usage: ping x.x.x.x"
    },
    {
        "exit",
        "Exit the shell.",
        "Usage: exit"
    },
};
// clang-format on

int main(int argc, char **argv) {
  int total_commands = sizeof(help_table) / sizeof(struct CommandHelp);

  if (argc > 1 && strcmp(argv[1], "-l") == 0) {
    printf("Available commands:\n");
    for (int i = 0; i < total_commands; i++) {
      printf("  %s\n", help_table[i].name);
    }
    return -1;
  }

  // Case 2: Specific command help (e.g., "help ls")
  if (argc > 1) {
    for (int i = 0; i < total_commands; i++) {
      if (strcmp(argv[1], help_table[i].name) == 0) {
        printf("Command: %s\n", help_table[i].name);
        printf("Description: %s\n", help_table[i].summary);
        printf("\n%s\n", help_table[i].details);
        return -1;
      }
    }
    printf("Error: Command '%s' not found.\n", argv[1]);
    return -1;
  }

  printf("Type 'help [command]' for more info, or 'help -l' to list all.\n\n");
  for (int i = 0; i < total_commands; i++) {
    printf("%-10s - %s\n", help_table[i].name, help_table[i].summary);
  }
  return 0;
}
