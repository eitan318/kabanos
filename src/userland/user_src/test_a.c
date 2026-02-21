#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
  printf("hello");
  char key_buf[3];
  for (;;) {
    read(FD_STDIN, key_buf, 1);
    write(FD_STDOUT, key_buf, 1);
  }
}
