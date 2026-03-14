#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void handle_error(const char *path)
{
  switch(errno) {
    case EEXIST:
      printf("mkdir: cannot create directory '%s': File exists\n", path);
      break;
    case ENOENT:
      printf("mkdir: cannot create directory '%s': No such file or directory\n", path);
      break;

    default:
      printf("mkdir failed: unknown error\n");
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: mkdir <path>\n");
    return 1;
  }

  int total_length = strlen(argv[1]) + 2; // +2 - 1 - null terminator, 1 - for '/'
  char *path = (char*)calloc(total_length, sizeof(char)); // +1 for null terminator

  if (argv[1][0] != '/') {
    strcat(path, "/");
  }
  strcat(path, argv[1]);

  if (mkdir(path, 0) == 0) {
    free(path);
    return 0;
  }

  if (errno)
    handle_error(argv[1]);

  free(path);
  return 1;
}