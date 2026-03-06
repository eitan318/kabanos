/** @file cmdline.h
 * @brief Kernel command line parsing utilities.
 */

#pragma once
#include "klib/stddef.h"

/** * @brief Finds a key in the cmdline and returns a pointer to its value.
 * * If the key is a flag (no '='), it returns a pointer to the key itself.
 * If the key has a value (key=val), it returns a pointer to the start of 'val'.
 * * @param cmdline The raw command line string.
 * @param key The key to search for.
 * @return Pointer to the value/flag within the cmdline string, or NULL if not
 * found.
 */
const char *cmdline_get_arg(const char *cmdline, const char *key);

/** * @brief Copies a value from the cmdline into a null-terminated buffer.
 * * Copies characters from src until a space or null-terminator is encountered.
 * * @param dest The destination buffer.
 * @param src The pointer returned by cmdline_get_arg.
 * @param max_len Maximum bytes to write to dest (including null terminator).
 */
void cmdline_copy_value(char *dest, const char *src, size_t max_len);
