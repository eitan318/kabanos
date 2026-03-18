#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("No argument provided.\n");
  }
  for (int i = 0; i < 150; i++) {
    printf("%s", argv[1]);
    fflush(stdout);
    sleep(0.2);
  }

  return 0;
}
