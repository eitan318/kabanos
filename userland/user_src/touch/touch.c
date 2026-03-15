#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int create(const char *path);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: touch <file name>\n");
    return 1;
  }

  int total_length = strlen(argv[1]) + 2;
  char *path = (char *)calloc(total_length, sizeof(char));

  if (argv[1][0] != '/') {
    strcat(path, "/");
  }
  strcat(path, argv[1]);

  if (create(path) == 0) {
    free(path);
    return 0;
  }

  printf("touch: cannot create file '%s'\n", argv[1]);
  free(path);
  return 1;
}