/** @file cmdline.h
 * @brief Kernel command line parsing utilities.
 */

#pragma once
#include "klib/stddef.h"

/**
 * @brief Finds a key's value without allocating memory.
 * @note Result is NOT null-terminated. Use val_len for bounds.
 */
const char *cmdline_get_arg(const char *cmdline, const char *key,
                            size_t *val_len);

/**
 * @brief Finds a key's value and returns a null-terminated copy.
 * @note Caller MUST call free the str.
 */
char *cmdline_get_arg_copy(const char *cmdline, const char *key);
