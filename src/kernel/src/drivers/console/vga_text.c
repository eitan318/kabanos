#include "drivers/console/vga_text.h"
#include "hal.h"
#include "mm/memdefs.h"
#include "modules.h"

const unsigned SCREEN_WIDTH = 80;
const unsigned SCREEN_HEIGHT = 25;

static uint8_t *g_vga_buf = (uint8_t *)VGA_SCREEN_BUF;
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static uint8_t g_color = VGA_DEFAULT_COLOR;

void vga_write_char(int x, int y, char c) {
  g_vga_buf[2 * (y * SCREEN_WIDTH + x)] = c;
}

void vga_write_color(int x, int y, uint8_t color) {
  g_vga_buf[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

char vga_read_char(int x, int y) {
  return g_vga_buf[2 * (y * SCREEN_WIDTH + x)];
}

uint8_t vga_read_color(int x, int y) {
  return g_vga_buf[2 * (y * SCREEN_WIDTH + x) + 1];
}

void vga_cursor_set(int x, int y) {
  int pos = y * SCREEN_WIDTH + x;
  hal_out8(0x3D4, 0x0F);
  hal_out8(0x3D5, (uint8_t)(pos & 0xFF));
  hal_out8(0x3D4, 0x0E);
  hal_out8(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_scroll(int lines) {
  for (int y = lines; y < SCREEN_HEIGHT; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      vga_write_char(x, y - lines, vga_read_char(x, y));
      vga_write_color(x, y - lines, vga_read_color(x, y));
    }
  for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      vga_write_char(x, y, '\0');
      vga_write_color(x, y, g_color);
    }
  g_cursor_y -= lines;
}

void vga_clear(void) {
  for (int y = 0; y < SCREEN_HEIGHT; y++)
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      vga_write_char(x, y, '\0');
      vga_write_color(x, y, g_color);
    }
  g_cursor_x = 0;
  g_cursor_y = 0;
  vga_cursor_set(0, 0);
}

static int vga_module_init(module_t *self) {
  vga_clear();
  return 0;
}

static const char *vga_deps[] = {"hal", NULL};
ITER_MODULE(vga) = {
    .name = "vga",
    .required_modules_names = vga_deps,
    .init = &vga_module_init,
};
