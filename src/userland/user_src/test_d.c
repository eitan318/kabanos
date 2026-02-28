#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
  printf("hello\n");
  char key_buf[3];
  for (;;) {
    read(STDIN_FILENO, key_buf, 1);
    write(STDOUT_FILENO, key_buf, 1);
  }
}
