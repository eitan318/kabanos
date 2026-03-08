#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

DIR *opendir(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return NULL;
  }

  DIR *dir = malloc(sizeof(DIR));
  if (!dir) {
    close(fd);
    return NULL;
  }

  printf("dir is not 0");

  dir->fd = fd;
  dir->buf_pos = 0;
  dir->buf_end = 0;

  return dir;
}
