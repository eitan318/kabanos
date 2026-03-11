#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("usage cp <file> <file>\n");
    return 1;
  }
  int src_fd, dst_fd;
  ssize_t n_read;
  char buffer[BUF_SIZE];

  src_fd = open(argv[1], O_RDONLY);
  dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);

  while ((n_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
    if (write(dst_fd, buffer, n_read) != n_read) {
      perror("Write error");
      return 1;
    }
  }

  close(src_fd);
  close(dst_fd);
  return 0;
}
