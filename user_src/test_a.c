#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  char key_buf[3];
  for (;;) {
    read(FD_STDIN, key_buf, 1);
    write(FD_STDOUT, key_buf, 1);
  }
}
