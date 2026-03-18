#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handle_error(const char *path) {
  switch (errno) {
  case ENOENT:
    fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n",
            path);
    break;
  case EISDIR:
    fprintf(stderr, "rm: cannot remove '%s': Is a directory\n", path);
    break;
  case EACCES:
    fprintf(stderr, "rm: cannot remove '%s': Permission denied\n", path);
    break;
  default:
    fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
    break;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <path>\n", argv[0]);
    return 1;
  }

  if (unlink(argv[1]) == 0) {
    return 0;
  }

  handle_error(argv[1]);
  return 1;
}
