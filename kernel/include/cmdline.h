/** @file cmdline.h
 * @brief Kernel command line parsing utilities.
 */

#pragma once
#include "klib/stddef.h"

/**
 * @brief Finds a key's value without allocating memory.
 * @param val_len [out] Length of the returned value in bytes.
 * @return Pointer into @p cmdline, or NULL if @p key is absent.
 * @note The result is NOT null-terminated; use @p val_len for bounds.
 */
const char *cmdline_get_arg(const char *cmdline, const char *key,
                            size_t *val_len);

/**
 * @brief Finds a key's value and returns a null-terminated copy.
 * @return Newly allocated string, or NULL if @p key is absent.
 * @note Caller must release the returned string with kfree().
 */
char *cmdline_get_arg_copy(const char *cmdline, const char *key);
