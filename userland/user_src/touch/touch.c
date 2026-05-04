#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_error(const char *path) {
  switch (errno) {
  case EEXIST:
    printf("touch: cannot create file '%s': File exists\n", path);
    break;

  default:
    printf("errno: %d\n", errno);
    printf("touch failed: unknown error\n");
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file name>\n", argv[0]);
    return 1;
  }
  int fd = open(argv[1], O_WRONLY | O_CREAT, 0644);

  if (fd == -1) {
    perror(argv[1]);
    return 1;
  }

  close(fd);
  return 0;
}
