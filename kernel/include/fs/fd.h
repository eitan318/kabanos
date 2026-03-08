#pragma once

typedef int fd_t;
#include "vfs_internal.h"

#define MAX_FD 64
extern file_t *g_fd_table[];

void vfs_init_stdio(void);
