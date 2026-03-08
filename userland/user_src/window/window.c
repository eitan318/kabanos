#include <stdio.h>

// Helper to move cursor
void move_cursor(int x, int y) { printf("\033[%d;%dH", y, x); }

// Draw a window frame
void draw_window(int x, int y, int width, int height, char *title) {
  // Draw top border
  move_cursor(x, y);
  printf("┌─ %s ", title);
  for (int i = 0; i < width - 6 - (int)sizeof(title); i++)
    printf("─");
  printf("┐");

  // Draw sides
  for (int i = 1; i < height; i++) {
    move_cursor(x, y + i);
    printf("│");
    move_cursor(x + width, y + i);
    printf("│");
  }

  // Draw bottom
  move_cursor(x, y + height);
  printf("└");
  for (int i = 0; i < width - 1; i++)
    printf("─");
  printf("┘");
}

int main() {
  // 1. Clear the screen
  printf("\033[2J");

  // 2. Draw a "Background" or desktop
  for (int i = 0; i < 30; i++) {
    move_cursor(1, i);
    printf("\033[1;30m.\033[0m"); // Grey dots
  }

  // 3. Draw Window 1 (System Info)
  draw_window(5, 5, 40, 10, "System Info");
  move_cursor(7, 7);
  printf("OS: Arch Linux (Text Mode)");
  move_cursor(7, 8);
  printf("Kernel: Custom C-Shell");

  // 4. Draw Window 2 (The Pengwin)
  draw_window(50, 3, 30, 15, "Pengwin Art");
  move_cursor(52, 5);
  printf("  ⢠⣖⣢  "); // You can loop through your ASCII array here

  // Move cursor to bottom for input
  move_cursor(1, 25);
  printf("Shell > ");
  fflush(stdout);

  return 0;
}
