#include "unistd.h"
#include <stddef.h>

void main(void) {
  for (;;) {
    write(FD_STDOUT, "A", 1);
    // Simple delay loop so it doesn't fill the screen too fast
    for (volatile int i = 0; i < DELAY_LOOP; i++)
      ;
  }
}
