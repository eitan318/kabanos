#include <stdio.h>

int main(int argc, char *argv[]) {
  // \033[2J   : Clear the entire screen
  // \033[H    : Move cursor to home position (1,1)
  // We use fputs or printf followed by fflush to ensure it hits the screen
  // immediately
  printf("\033[2J\033[H");
  fflush(stdout);

  return 0;
}
