#include "stdio.h"
#include "string.h"

struct CommandHelp {
  char *name;    // The command (e.g., "ls")
  char *summary; // Brief one-liner
  char *details; // Longer explanation (the 'man' part)
};

struct CommandHelp help_table[] = {
    {"help", "Display information about built-in commands.",
     "Usage: help [-l] [command]"},
    {"ls", "List directory contents.",
     "Usage: ls [directory]\nOptions: -a (all), -l (long format)"},
    {"cd", "Change the working directory.",
     "Usage: cd [path]\nUse '..' to go up one level."},
    // Add more commands here...
};

void main(int argc, char **argv) {
  int total_commands = sizeof(help_table) / sizeof(struct CommandHelp);
  printf("argc:%d", argc);

  if (argc > 1 && strcmp(argv[1], "-l") == 0) {
    printf("Available commands:\n");
    for (int i = 0; i < total_commands; i++) {
      printf("  %s\n", help_table[i].name);
    }
    return;
  }

  // Case 2: Specific command help (e.g., "help ls")
  if (argc > 1) {
    for (int i = 0; i < total_commands; i++) {
      if (strcmp(argv[1], help_table[i].name) == 0) {
        printf("Command: %s\n", help_table[i].name);
        printf("Description: %s\n", help_table[i].summary);
        printf("\n%s\n", help_table[i].details);
        return;
      }
    }
    printf("Error: Command '%s' not found.\n", argv[1]);
    return;
  }

  printf("Type 'help [command]' for more info, or 'help -l' to list all.\n\n");
  for (int i = 0; i < total_commands; i++) {
    printf("%-10s - %s\n", help_table[i].name, help_table[i].summary);
  }
}
