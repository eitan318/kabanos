#include "drivers/console/console.h"
#include "drivers/console/vga_text.h"
#include "modules.h"
#include <klib/stdarg.h>

extern const unsigned SCREEN_WIDTH;
extern const unsigned SCREEN_HEIGHT;

static int g_con_x = 0;
static int g_con_y = 0;
static uint8_t g_con_color = VGA_DEFAULT_COLOR;

void con_set_color(uint8_t color) { g_con_color = color; }
uint8_t con_get_color(void) { return g_con_color; }

void con_cursor_get(int *x, int *y) {
  *x = g_con_x;
  *y = g_con_y;
}
void con_cursor_set(int x, int y) {
  g_con_x = x;
  g_con_y = y;
  vga_cursor_set(x, y);
}

static void con_advance(void) {
  if (g_con_x >= (int)SCREEN_WIDTH) {
    g_con_x = 0;
    g_con_y++;
  }
  if (g_con_y >= (int)SCREEN_HEIGHT)
    vga_scroll(1);
  vga_cursor_set(g_con_x, g_con_y);
}

void con_putc(char c) {
  switch (c) {
  case '\n':
    g_con_x = 0;
    g_con_y++;
    break;
  case '\r':
    g_con_x = 0;
    break;
  case '\t':
    /* advance to next 4-column tab stop */
    do {
      con_putc(' ');
    } while (g_con_x % 4 != 0);
    return;
  default:
    vga_write_char(g_con_x, g_con_y, c);
    vga_write_color(g_con_x, g_con_y, g_con_color);
    g_con_x++;
    break;
  }
  con_advance();
}

void con_backspace(void) {
  if (g_con_x > 0) {
    g_con_x--;
  } else if (g_con_y > 0) {
    /* wrap to end of previous line */
    g_con_y--;
    g_con_x = SCREEN_WIDTH - 1;
  } else {
    return; /* already at 0,0 */
  }
  vga_write_char(g_con_x, g_con_y, '\0');
  vga_write_color(g_con_x, g_con_y, g_con_color);
  vga_cursor_set(g_con_x, g_con_y);
}

void con_puts(const char *str) {
  while (*str)
    con_putc(*str++);
}

void con_putsn(const char *str, int n) {
  for (int i = 0; i < n && str[i]; i++)
    con_putc(str[i]);
}

void con_clear(void) {
  vga_clear();
  g_con_x = 0;
  g_con_y = 0;
}

void con_newline(void) { con_putc('\n'); }

static int con_module_init(module_t *self) {
  con_clear();
  return 0;
}

static const char *con_deps[] = {"vga", NULL};
ITER_MODULE(console) = {
    .name = "console",
    .required_modules_names = con_deps,
    .init = &con_module_init,
};
