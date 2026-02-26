#include "unistd.h"
#include <stddef.h>
#include <stdio.h>

int main() {
  for (;;) {
    printf("A");
    fflush(stdout);
    for (volatile int i = 0; i < DELAY_LOOP; i++)
      ;
  }
}
