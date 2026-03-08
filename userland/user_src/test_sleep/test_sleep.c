#include <stddef.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

void main() {
  printf("  Start sleeping\n");
  sleep(2);
  printf("  Finish sleeping\n");
}
