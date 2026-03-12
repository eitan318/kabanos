#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

extern int __myos_errno;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: mkdir <path>\n");
    return 1;
  }

  int total_length = strlen(argv[1]) + 2; // +2 - 1 - null terminator, 1 - for '/'
  char *path = (char*)calloc(total_length, sizeof(char)); // +1 for null terminator

  if (argv[1][0] != '/') {
    strcat(path, "/");
  }
  strcat(path, argv[1]);

  if (mkdir(path, 0) == 0) {
    free(path);
    return 0;
  }

  if (__myos_errno == EEXIST) {
    printf("mkdir: cannot create directory '%s': File exists\n", argv[1]);
    free(path);
    return 1;
  }

  printf("mkdir failed\n");
  free(path);
  return 1;
}