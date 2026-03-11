#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <file>\n", argv[0]);
    return 1;
  }

  FILE *fp = fopen(argv[1], "r");
  if (!fp) {
    perror("Error opening file");
    return 1;
  }

  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
    fwrite(buf, 1, n, stdout);
  }

  fclose(fp);
  return 0;
}
