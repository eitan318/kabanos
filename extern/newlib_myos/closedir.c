#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int closedir(DIR *dir) {
  close(dir->fd);

  free(dir);
}
