#include "unistd.h"
#include <stddef.h>
#include <stdio.h>

void main(void) {
  for (;;) {
    printf("B");
    fflush(stdout);

    for (volatile int i = 0; i < DELAY_LOOP; i++)
      ;
  }
}
