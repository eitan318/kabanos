#include "arch/i686/vga_text.h"
#include "drivers/keyboard/keyboard_driver.h"
#include "include/stdio.h"

void prompt_for_keyboard() {
  printf("Keyboard ready - start typing:\n");

  for (;;) {
    char c = kbd_char_get();
    if (c == 8) {
      vga_setcursor();
    } else if (c != 0) {
      printf("%c", c);
    }
  }
}
