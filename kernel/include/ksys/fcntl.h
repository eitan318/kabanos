#pragma once

// File access modes - These occupy the lowest 2 bits of the flags */
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002

#define O_ACCMODE 0x0003

// File creation and status flags
#define O_CREAT 0x0040
#define O_EXCL 0x0080
#define O_TRUNC 0x0200
#define O_APPEND 0x0400
#define O_NONBLOCK 0x0800
#define O_DIRECTORY 0x10000
