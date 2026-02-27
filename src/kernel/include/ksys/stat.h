#pragma once
typedef long ssize_t;    // Signed size type
typedef long long off_t; // Offset type (often 64-bit for large files)
typedef unsigned long
    ino_t; // Standard 32-bit or 64-bit unsigned intypedef long long off_t; //
           // Offset type (often 64-bit for large files)

/* File type masks */
#define S_IFMT 0170000 /* Bitmask for the file type bit field */

#define S_IFSOCK 0140000 /* Socket */
#define S_IFLNK 0120000  /* Symbolic link */
#define S_IFREG 0100000  /* Regular file */
#define S_IFBLK 0060000  /* Block device */
#define S_IFDIR 0040000  /* Directory */
#define S_IFCHR 0020000  /* Character device */
#define S_IFIFO 0010000  /* FIFO */

/* Helper Macros to check types */
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#define S_ISLNK(m) (((m)&S_IFMT) == S_IFLNK)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
