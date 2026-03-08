#pragma once
#include "klib/stdint.h"

void con_putc(char c);
void con_backspace(void);

void con_puts(const char *str);
void con_putsn(const char *str, int n);

void con_clear(void);
void con_newline(void);

void con_set_color(uint8_t color);
uint8_t con_get_color(void);

void con_cursor_get(int *x, int *y);
void con_cursor_set(int x, int y);
