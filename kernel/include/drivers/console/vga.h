#pragma once
#include "klib/stdint.h"

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

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define VGA_MAKE_COLOR(fg, bg) (((bg) << 4) | (fg))

/* raw cell access */
void vga_write_char(int x, int y, char c);
void vga_write_color(int x, int y, uint8_t color);
char vga_read_char(int x, int y);
uint8_t vga_read_color(int x, int y);

void vga_scroll(int lines, uint8_t back_color);
void vga_clear(uint8_t back_color);

/* hardware cursor */
void vga_cursor_set(int x, int y);
