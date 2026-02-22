#include "unistd.h"
#include <stddef.h>
#include <stdio.h>

void main(void) {
  for (;;) {
    printf("A");
    fflush(stdout);
    execve("test_a.elf", NULL, NULL);
  }
}
