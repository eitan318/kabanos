#pragma once
/* File access modes - These occupy the lowest 2 bits of the flags */
#define O_RDONLY 0x0000 /* Read-only */
#define O_WRONLY 0x0001 /* Write-only */
#define O_RDWR 0x0002   /* Read-write */

/* Mask for access modes */
#define O_ACCMODE 0x0003

/* File creation and status flags */
#define O_CREAT 0x0040      /* Create file if it doesn't exist */
#define O_EXCL 0x0080       /* Error if O_CREAT and file exists */
#define O_TRUNC 0x0200      /* Truncate file to zero length */
#define O_APPEND 0x0400     /* Append to end of file */
#define O_NONBLOCK 0x0800   /* Non-blocking I/O */
#define O_DIRECTORY 0x10000 /* Must be a directory */
