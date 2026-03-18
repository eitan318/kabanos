#pragma once

#include <stdint.h>

// VGA color constants
#define VGA_COLOR_BLACK 0x0
#define VGA_COLOR_BLUE 0x1
#define VGA_COLOR_GREEN 0x2
#define VGA_COLOR_CYAN 0x3
#define VGA_COLOR_RED 0x4
#define VGA_COLOR_MAGENTA 0x5
#define VGA_COLOR_BROWN 0x6
#define VGA_COLOR_LIGHT_GREY 0x7
#define VGA_COLOR_DARK_GREY 0x8
#define VGA_COLOR_LIGHT_BLUE 0x9
#define VGA_COLOR_LIGHT_GREEN 0xA
#define VGA_COLOR_LIGHT_CYAN 0xB
#define VGA_COLOR_LIGHT_RED 0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_YELLOW 0xE
#define VGA_COLOR_WHITE 0xF

#define VGA_MAKE_COLOR(fg, bg) ((bg << 4) | (fg))
#define VGA_DEFAULT_COLOR VGA_MAKE_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)

// Low-level hardware functions
void vga_putchr(int x, int y, char c);
void vga_putcolor(int x, int y, uint8_t color);
char vga_getchr(int x, int y);
uint8_t vga_getcolor(int x, int y);
void vga_setcursor(int x, int y);
void vga_scrollback(int lines);

// High-level output functions (use current color state)
void vga_putc(char c);
void vga_puts(const char *str);
void vga_clrscr();

// Color management
void vga_set_color(uint8_t color);
uint8_t vga_get_color(void);
