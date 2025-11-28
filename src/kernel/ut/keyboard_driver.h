#include "drivers/keyboard/keyboard_driver.h"
#include "include/stdio.h"

void prompt_for_keyboard() {
  while (1) {
    printf("ctrl+c testing - stop the loop\n");

    // Simple busy-wait delay
    for (volatile int i = 0; i < 100000000; i++)
      ;

    // Check keyboard input
    char c = kbd_char_get();
    if (c != 0) {
      if (c == 0x03) { // Ctrl+C
        printf("Ctrl+C detected - breaking loop!\n");
        break;
      }
    }
  }

  printf("Keyboard ready - start typing:\n");

  for (;;) {
    char c = kbd_char_get();
    if (c != 0) {
      printf("%c", c);
    }
  }
}
