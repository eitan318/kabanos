// ============================================================================
// vga_text.c - VGA Text Mode Driver Implementation
// ============================================================================
#include "vga_text.h"
#include "hal.h"
#include "memory_management/memdefs.h"

const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;

// Hardware state
static uint8_t *g_ScreenBuffer = (uint8_t *)VGA_SCREEN_BUF;
static int g_ScreenX = 0;
static int g_ScreenY = 0;
static uint8_t g_CurrentColor = VGA_DEFAULT_COLOR; // Current color state

// Low-level hardware access
void vga_putchr(int x, int y, char c) {
  g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

void vga_putcolor(int x, int y, uint8_t color) {
  g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

char vga_getchr(int x, int y) {
  return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

uint8_t vga_getcolor(int x, int y) {
  return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}

void vga_setcursor(int x, int y) {
  int pos = y * SCREEN_WIDTH + x;
  io_write8(0x3D4, 0x0F);
  io_write8(0x3D5, (uint8_t)(pos & 0xFF));
  io_write8(0x3D4, 0x0E);
  io_write8(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_scrollback(int lines) {
  for (int y = lines; y < SCREEN_HEIGHT; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      vga_putchr(x, y - lines, vga_getchr(x, y));
      vga_putcolor(x, y - lines, vga_getcolor(x, y));
    }
  for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      vga_putchr(x, y, '\0');
      vga_putcolor(x, y, g_CurrentColor);
    }
  g_ScreenY -= lines;
}

// Color management - VGA driver maintains the current color
void vga_set_color(uint8_t color) { g_CurrentColor = color; }

uint8_t vga_get_color(void) { return g_CurrentColor; }

// Clear screen
void vga_clrscr() {
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      vga_putchr(x, y, '\0');
      vga_putcolor(x, y, g_CurrentColor);
    }
  }
  g_ScreenX = 0;
  g_ScreenY = 0;
  vga_setcursor(g_ScreenX, g_ScreenY);
}

// Character output - uses current color state
void vga_putc(char c) {
  switch (c) {
  case '\n':
    g_ScreenX = 0;
    g_ScreenY++;
    break;
  case '\t':
    for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
      vga_putc(' ');
    break;
  case '\r':
    g_ScreenX = 0;
    break;
  default:
    vga_putchr(g_ScreenX, g_ScreenY, c);
    vga_putcolor(g_ScreenX, g_ScreenY, g_CurrentColor);
    g_ScreenX++;
    break;
  }
  if (g_ScreenX >= SCREEN_WIDTH) {
    g_ScreenY++;
    g_ScreenX = 0;
  }
  if (g_ScreenY >= SCREEN_HEIGHT)
    vga_scrollback(1);
  vga_setcursor(g_ScreenX, g_ScreenY);
}

// String output with ANSI escape sequence support
void vga_puts(const char *str) {
  const char *s = str;
  while (*s) {
    vga_putc(*s);
    s++;
  }
}
