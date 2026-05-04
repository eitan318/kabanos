#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define BUF_SIZE 512

static char *build_path(const char *arg) {
  int len = strlen(arg) + 2;
  char *path = (char *)calloc(len, sizeof(char));
  if (arg[0] != '/')
    strcat(path, "/");
  strcat(path, arg);
  return path;
}

static const char *basename(const char *path) {
  const char *p = path;
  const char *last = path;
  while (*p) {
    if (*p == '/')
      last = p + 1;
    p++;
  }
  return last;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: cp <src> <dst>\n");
    return 1;
  }

  char *src_path = build_path(argv[1]);
  char *dst_path = build_path(argv[2]);

  // check if dst is a directory
  DIR *d = opendir(dst_path);
  if (d) {
    closedir(d);
    // dst is a dir — append src filename to it
    const char *fname = basename(src_path);
    int new_len = strlen(dst_path) + strlen(fname) + 2;
    char *new_dst = (char *)calloc(new_len, sizeof(char));
    strcat(new_dst, dst_path);
    strcat(new_dst, "/");
    strcat(new_dst, fname);
    free(dst_path);
    dst_path = new_dst;
  }

  create(dst_path);

  int src_fd = open(src_path, 0);
  if (src_fd < 0) {
    printf("cp: cannot open '%s'\n", argv[1]);
    free(src_path);
    free(dst_path);
    return 1;
  }

  int dst_fd = open(dst_path, 1);
  if (dst_fd < 0) {
    printf("cp: cannot open '%s'\n", dst_path);
    close(src_fd);
    free(src_path);
    free(dst_path);
    return 1;
  }

  char buffer[BUF_SIZE];
  size_t n_read;
  while ((n_read = read(src_fd, buffer, BUF_SIZE)) > 0) {
    if (write(dst_fd, buffer, n_read) != n_read) {
      printf("cp: write error\n");
      close(src_fd);
      close(dst_fd);
      free(src_path);
      free(dst_path);
      return 1;
    }
  }

  close(src_fd);
  close(dst_fd);
  free(src_path);
  free(dst_path);
  return 0;
}