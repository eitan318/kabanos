#include "unistd.h"
#include <stddef.h>
#include <stdio.h>

int main() {
  printf("Enter a num:\n");
  char buf[5];
  read(STDIN_FILENO, buf, 3);
  printf("The num is: %s", buf);
  return 0;
}
