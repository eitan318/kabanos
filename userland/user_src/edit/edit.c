#include "stddef.h"
#include "stdio.h"

int main(int argc, char **argv) {
  printf("argc: %d", argc);
  printf("argv: %s", argv);
  if (argc < 2)
    return 1;

  char buf[100000];

  FILE *fp;
  fp = fopen(argv[1], "r");
  if (fp) {
    fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    printf("Content: \n%s\n", buf);
  }

  printf("enter new text:");

  size_t n = fread(buf, 1, sizeof(buf), stdin);
  buf[n] = '\0';

  fp = fopen(argv[1], "w");
  if (fp) {
    fwrite(buf, 1, sizeof(buf), fp);
    fclose(fp);
    printf("\nSaved\n");
  }
  return 0;
}
