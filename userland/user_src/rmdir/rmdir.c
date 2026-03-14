#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void handle_error(const char *path)
{
  switch(errno) {
    case ENOENT:
      printf("rmdir: failed to remove '%s': No such file or directory\n", path);
      break;
    case ENOTEMPTY:
      printf("rmdir: failed to remove '%s': Directory not empty\n", path);
      break;
    case ENOTDIR:
      printf("rmdir: failed to remove '%s': Not a directory\n", path);
      break;
    
    default:
      printf("errno: %d\n", errno);
      printf("ENOTEMPTY: %d\n", ENOTEMPTY);
      printf("rmdir failed: unknown error\n");
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: rmdir <path>\n");
    return 1;
  }

  int total_length = strlen(argv[1]) + 2; // +2 - 1 - null terminator, 1 - for '/'
  char *path = (char*)calloc(total_length, sizeof(char)); // +1 for null terminator

  if (argv[1][0] != '/') {
    strcat(path, "/");
  }
  strcat(path, argv[1]);

  if (rmdir(path) == 0) {
    free(path);
    return 0;
  }

  if (errno)
    handle_error(argv[1]);

  free(path);
  return 1;
}