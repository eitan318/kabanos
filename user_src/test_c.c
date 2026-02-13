#include "include/posix.h"
#include <stddef.h>

void _start(void) {
  for (;;) {
    write(FD_STDOUT, "C", 1);
  }
}
