#pragma once
#include "klib/stdint.h"
int sys_ioctl(int fd, uint32_t request, void *arg);
