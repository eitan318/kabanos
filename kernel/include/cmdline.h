#pragma once
#include "klib/stddef.h"
const char *cmdline_get_arg(const char *cmdline, const char *key);
void cmdline_copy_value(char *dest, const char *src, size_t max_len);
