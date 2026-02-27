#pragma once

#define STDIN_FILENO 0  /* Standard input */
#define STDOUT_FILENO 1 /* Standard output */
#define STDERR_FILENO 2 /* Standard error */

/* --- Seek Constants (whence) --- */
/* These match your fat_seek implementation switches */
#ifndef SEEK_SET
#define SEEK_SET 0 /* Seek from beginning of file */
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1 /* Seek from current position */
#endif

#ifndef SEEK_END
#define SEEK_END 2 /* Seek from end of file */
#endif

/* --- Access Modes (for access()) --- */
#define R_OK 4 /* Test for read permission */
#define W_OK 2 /* Test for write permission */
#define X_OK 1 /* Test for execute permission */
#define F_OK 0 /* Test for existence of file */

/* --- Path and Name Limits --- */
/* Note: FAT has its own limits, but these are VFS-wide */
#define PATH_MAX 4096
#define NAME_MAX 255
