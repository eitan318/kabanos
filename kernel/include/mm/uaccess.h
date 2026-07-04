/**
 * @file uaccess.h
 * @brief Safe data transfer between kernel and user address spaces.
 */
#pragma once
#include "stddef.h"

/**
 * @brief Copies @p size bytes from kernel memory into user memory.
 * @return 0 on success, negative errno if the user range is invalid.
 */
int uaccess_copy_to_user(void *user_dst, const void *kernel_src, size_t size);

/**
 * @brief Copies @p size bytes from user memory into kernel memory.
 * @return 0 on success, negative errno if the user range is invalid.
 */
int uaccess_copy_from_user(void *kernel_dst, const void *user_src, size_t size);
