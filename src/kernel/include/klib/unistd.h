#pragma once

// File descriptors of std streams
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define STDDEBUG_FILENO 3

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

// Access Modes (for access())
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0

// Path and name vfs limits
#define PATH_MAX 4096
#define NAME_MAX 255
