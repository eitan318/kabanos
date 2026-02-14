#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  for (;;) {
    write(FD_STDOUT, "B", 1);
    // Simple delay loop so it doesn't fill the screen too fast
    for (volatile int i = 0; i < DELAY_LOOP; i++)
      ;
  }
}
