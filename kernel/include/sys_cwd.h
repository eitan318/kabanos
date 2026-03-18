#pragma once
#include "stddef.h"

int sys_getcwd(char *buf, size_t size);
int sys_chdir(const char *path);
