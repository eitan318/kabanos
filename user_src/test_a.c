#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  char key_buf[3];
  for (;;) {

    read(FD_STDIN, key_buf, 3);
    write(FD_STDOUT, "A", 1);
    // Simple delay loop so it doesn't fill the screen too fast
    for (volatile int i = 0; i < DELAY_LOOP; i++)
      ;
  }
}
