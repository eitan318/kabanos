#include "unistd.h"
#include <stddef.h>
#include <stdio.h>

int main() {
  for (;;) {
    printf("C");
    fflush(stdout);
    for (volatile int i = 0; i < DELAY_LOOP; i++)
      ;

    execve("test_c.elf", NULL, NULL);
  }
}
