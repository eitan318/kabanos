/**
 * @file console.h
 * @brief Text console on top of the VGA driver: cursor tracking, colors,
 *        scrolling and newline handling.
 */
#pragma once
#include "klib/stdint.h"

/** @brief Writes one character at the cursor, handling control characters. */
void con_putc(char c);

/** @brief Erases the character before the cursor. */
void con_backspace(void);

void con_puts(const char *str);

/** @brief Writes at most @p n characters of @p str. */
void con_putsn(const char *str, int n);

void con_clear(void);
void con_newline(void);

/** @brief Sets the VGA attribute byte used for subsequent output. */
void con_set_color(uint8_t color);
uint8_t con_get_color(void);

void con_cursor_get(int *x, int *y);
void con_cursor_set(int x, int y);
