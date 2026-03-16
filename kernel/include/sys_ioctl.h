#pragma once
#include "klib/stdint.h"
int sys_ioctl(int fd, unsigned long request, void *arg);
