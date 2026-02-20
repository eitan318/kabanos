#include "unistd.h"
#include <stddef.h>

int main(void) {
  write(FD_STDIN, "hello", 5);
  char key_buf[3];
  for (;;) {
    read(FD_STDIN, key_buf, 1);
    write(FD_STDOUT, key_buf, 1);
  }
}
