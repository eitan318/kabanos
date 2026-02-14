#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  while (1) {
    write(FD_STDOUT, "HELLO!", 1);
    sleep(4);
  }
}
