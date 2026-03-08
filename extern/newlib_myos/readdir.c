#include <dirent.h>
#include <string.h>
#include <unistd.h>

extern int getdents(int fd, void *buf, unsigned int size);

struct dirent *readdir(DIR *dir) {
  if (dir->buf_pos >= dir->buf_end) {

    int n = getdents(dir->fd, dir->buffer, sizeof(dir->buffer));

    if (n <= 0)
      return NULL;

    dir->buf_pos = 0;
    dir->buf_end = n;
  }

  struct dirent *d = (struct dirent *)(dir->buffer + dir->buf_pos);

  dir->buf_pos += sizeof(struct dirent);

  memcpy(&dir->current, d, sizeof(struct dirent));

  return &dir->current;
}
