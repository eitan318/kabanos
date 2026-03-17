#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

void handle_error(const char *path) {
  switch (errno) {
  case EEXIST:
    fprintf(stderr, "mkdir: cannot create directory '%s': File exists\n", path);
    break;
  case ENOENT:
    fprintf(stderr,
            "mkdir: cannot create directory '%s': No such file or directory\n",
            path);
    break;
  case EACCES:
    fprintf(stderr, "mkdir: cannot create directory '%s': Permission denied\n",
            path);
    break;
  default:
    fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", path,
            strerror(errno));
    break;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <path>\n", argv[0]);
    return 1;
  }

  if (mkdir(argv[1], 0755) == 0) {
    return 0;
  }

  handle_error(argv[1]);
  return 1;
}
