#pragma once
#include "stddef.h"
int uaccess_copy_to_user(void *user_dst, const void *kernel_src, size_t size);
int uaccess_copy_from_user(void *kernel_dst, const void *user_src, size_t size);
