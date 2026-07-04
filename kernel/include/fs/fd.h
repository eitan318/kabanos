/**
 * @file fd.h
 * @brief Global file descriptor table.
 */
#pragma once

typedef int fd_t;
#include "vfs_internal.h"

#define MAX_FD 64

/** @brief Open-file table indexed by fd; NULL entries are free. */
extern file_t *g_fd_table[];

/** @brief Sets up fds 0-2 (stdin/stdout/stderr) on the console device. */
void vfs_init_stdio(void);
