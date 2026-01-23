#include "drivers/keyboard.h"
#include "stdio.h"

void prompt_for_keyboard() {
  printf("Keyboard ready - start typing:\n");

  for (;;) {
    char c = kbd_char_get();
    if (c != 0) {
      if (c == 0x03) { // Ctrl+C
        printf("Ctrl+C detected - breaking loop!\n");
        return;
      }
      printf("%c", c);
    }
  }
}
