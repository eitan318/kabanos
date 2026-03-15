#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

void handle_error(const char *path) {
  switch (errno) {
    case ENOENT:
      printf("rm: cannot remove '%s': No such file or directory\n", path);
      break;
    case EISDIR:
      printf("rm: cannot remove '%s': Is a directory\n", path);
      break;
    default:
      printf("rm: cannot remove '%s': errno %d\n", path, errno);
      break;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: rm <path>\n");
    return 1;
  }

  int total_length = strlen(argv[1]) + 2;
  char *path = (char *)calloc(total_length, sizeof(char));

  if (argv[1][0] != '/') {
    strcat(path, "/");
  }
  strcat(path, argv[1]);

  if (unlink(path) == 0) {
    free(path);
    return 0;
  }

  handle_error(argv[1]);
  free(path);
  return 1;
}