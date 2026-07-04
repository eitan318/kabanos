/**
 * @file console.c
 * @brief Text console: cursor, colors, scrolling on top of the VGA driver.
 */
#include "drivers/console/console.h"
#include "drivers/console/vga.h"
#include "modules.h"
#include <klib/stdarg.h>
#include <klib/stdbool.h>

// State Machine Definitions for ANSI ESC sequences
typedef enum { STATE_NORMAL, STATE_BRACKET, STATE_PARAMS } con_state_t;

#define LIGHT_MODE VGA_MAKE_COLOR(VGA_COLOR_BLACK, VGA_COLOR_WHITE)
#define DARK_MODE VGA_MAKE_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)

typedef enum { CON_MODE_DARK, CON_MODE_LIGHT } con_mode_t;
static con_mode_t g_con_mode = CON_MODE_DARK;

static uint8_t ansi_to_vga_lookup_dark[] = {
    VGA_COLOR_BLACK, VGA_COLOR_RED,     VGA_COLOR_GREEN, VGA_COLOR_BROWN,
    VGA_COLOR_BLUE,  VGA_COLOR_MAGENTA, VGA_COLOR_CYAN,  VGA_COLOR_LIGHT_GREY};

static uint8_t ansi_to_vga_lookup_light[] = {
    VGA_COLOR_BLACK, VGA_COLOR_RED,     VGA_COLOR_GREEN, VGA_COLOR_BROWN,
    VGA_COLOR_BLUE,  VGA_COLOR_MAGENTA, VGA_COLOR_CYAN,  VGA_COLOR_LIGHT_GREY};

static uint8_t *ansi_to_vga_lookup(void) {
  return g_con_mode == CON_MODE_DARK ? ansi_to_vga_lookup_dark
                                     : ansi_to_vga_lookup_light;
}

static con_state_t g_state = STATE_NORMAL;
static int g_params[4];
static int g_param_idx = 0;

static int g_con_x = 0;
static int g_con_y = 0;
static int g_saved_x = 0;
static int g_saved_y = 0;
static uint8_t g_con_color = 0;

static uint8_t g_con_default_color;

void con_cursor_get(int *x, int *y) {
  *x = g_con_x;
  *y = g_con_y;
}

void con_cursor_set(int x, int y) {
  if (x < 0)
    x = 0;
  if (x >= (int)SCREEN_WIDTH)
    x = SCREEN_WIDTH - 1;
  if (y < 0)
    y = 0;
  if (y >= (int)SCREEN_HEIGHT)
    y = SCREEN_HEIGHT - 1;

  g_con_x = x;
  g_con_y = y;
  vga_cursor_set(x, y);
}

static void handle_csi_command(char c) {
  switch (c) {
  case 's': // Save Cursor: \e[s
    g_saved_x = g_con_x;
    g_saved_y = g_con_y;
    break;

  case 'u': // Restore Cursor: \e[u
    con_cursor_set(g_saved_x, g_saved_y);
    break;
  case 'H': // Cursor Home / Move: \e[row;colH
  case 'f':
    // ANSI is 1-based, VGA is 0-based
    con_cursor_set(g_params[1] - 1, g_params[0] - 1);
    break;

  case 'J': // Clear Screen: \e[2J
    if (g_params[0] == 2) {
      con_clear();
    }
    break;

  case 'K': // Clear Line: \e[K
    for (int i = g_con_x; i < (int)SCREEN_WIDTH; i++) {
      vga_write_char(i, g_con_y, ' ');
      vga_write_color(i, g_con_y, g_con_color);
    }
    break;

  case 'm': // Select Graphic Rendition (Color)
    for (int i = 0; i <= g_param_idx; i++) {
      int p = g_params[i];
      if (p == 0) {
        g_con_color = g_con_default_color;
      } else if (p >= 30 && p <= 37) {
        g_con_color = (g_con_color & 0xF0) | ansi_to_vga_lookup()[p - 30];
      } else if (p >= 40 && p <= 47) {
        g_con_color =
            (g_con_color & 0x0F) | (ansi_to_vga_lookup()[p - 40] << 4);
      }
    }
    break;
  }
}

void con_putc(char c) {
  if (g_state == STATE_NORMAL) {
    if (c == '\033') {
      g_state = STATE_BRACKET;
      return;
    }
  } else if (g_state == STATE_BRACKET) {
    if (c == '[') {
      g_state = STATE_PARAMS;
      g_params[0] = g_params[1] = g_params[2] = g_params[3] = 0;
      g_param_idx = 0;
      return; // Correct
    } else if (c == '7') {
      g_saved_x = g_con_x;
      g_saved_y = g_con_y;
      g_state = STATE_NORMAL;
      return; // ADD THIS: Prevent printing '7'
    } else if (c == '8') {
      con_cursor_set(g_saved_x, g_saved_y);
      g_state = STATE_NORMAL;
      return; // ADD THIS: Prevent printing '8'
    } else {
      g_state = STATE_NORMAL;
      // return; // Optional: return here if you want to swallow unknown ESC
      // sequences
    }
  } else if (g_state == STATE_PARAMS) {
    if (c >= '0' && c <= '9') {
      g_params[g_param_idx] = g_params[g_param_idx] * 10 + (c - '0');
      return;
    } else if (c == ';') {
      if (g_param_idx < 3)
        g_param_idx++;
      return;
    } else if (c == '?') {
      // Just ignore the '?' for now so it doesn't break the state
      return;
    } else {
      handle_csi_command(c);
      g_state = STATE_NORMAL;
      return;
    }
  }
  // Handle standard character printing
  if (c == '\n') {
    g_con_x = 0;
    g_con_y++;
  } else if (c == '\r') {
    g_con_x = 0;
  } else if (c == '\t') {
    do {
      con_putc(' ');
    } while (g_con_x % 4 != 0);
    return; // Don't call con_advance again
  } else if (c == '\b') {
    con_backspace();
    return;
  } else {
    vga_write_char(g_con_x, g_con_y, c);
    vga_write_color(g_con_x, g_con_y, g_con_color);
    g_con_x++;
  }

  // Single point of truth for wrapping and scrolling
  if (g_con_x >= (int)SCREEN_WIDTH) {
    g_con_x = 0;
    g_con_y++;
  }

  if (g_con_y >= (int)SCREEN_HEIGHT) {
    vga_scroll(1, g_con_color);
    g_con_y = SCREEN_HEIGHT - 1;
  }

  vga_cursor_set(g_con_x, g_con_y);
}

void con_backspace(void) {
  if (g_con_x > 0) {
    g_con_x--;
  } else if (g_con_y > 0) {
    g_con_y--;
    g_con_x = SCREEN_WIDTH - 1;
  } else {
    return;
  }
  // Use a real space to clear the character
  vga_write_char(g_con_x, g_con_y, ' ');
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
  vga_clear(g_con_color);
  g_con_x = 0;
  g_con_y = 0;
  vga_cursor_set(0, 0);
}

void con_newline(void) { con_putc('\n'); }

static int con_module_init(module_t *self) {
  g_con_mode = CON_MODE_DARK;
  g_con_default_color = g_con_mode == CON_MODE_LIGHT ? LIGHT_MODE : DARK_MODE;
  g_con_color = g_con_default_color;
  con_clear();
  return 0;
}

static const char *con_deps[] = {"vga", NULL};
ITER_MODULE(console) = {
    .name = "console",
    .required_modules_names = con_deps,
    .init = &con_module_init,
};
