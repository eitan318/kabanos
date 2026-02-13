#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  char key_buf[3];

  write(FD_STDOUT, "Process A waiting for key...\n", 29);
  read(FD_STDIN, key_buf, 3);
  write(FD_STDOUT, "Process A woke up!\n", 19);
  for (;;)
    ;
}
