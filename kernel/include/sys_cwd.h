/**
 * @file sys_cwd.h
 * @brief Current-working-directory syscalls.
 */
#pragma once
#include "stddef.h"

/**
 * @brief Copies the absolute path of the current working directory into
 *        @p buf.
 * @param buf Destination buffer.
 * @param size Size of @p buf in bytes.
 * @return 0 on success, negative errno on failure (e.g. -ERANGE if the
 *         path does not fit).
 */
int sys_getcwd(char *buf, size_t size);

/**
 * @brief Changes the current working directory of the calling process.
 * @param path Path of the new working directory.
 * @return 0 on success, negative errno on failure.
 */
int sys_chdir(const char *path);
