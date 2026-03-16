#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

#include <sys/_types.h>
#include <sys/reent.h>

#ifdef __cplusplus
extern "C" {
#endif

/* * The kernel uses these values. We define the internal _F flags
 * so Newlib's default fcntl.h (which uses them to define O_*)
 * stays in sync.
 */
#define _FREAD 0x0001
#define _FWRITE 0x0002
#define _FAPPEND 0x0400
#define _FCREAT 0x0040
#define _FTRUNC 0x0200
#define _FEXCL 0x0080
#define _FNONBLOCK 0x0800

/* Now define the O_ flags to exactly match your kernel */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_ACCMODE 3

#define O_CREAT _FCREAT
#define O_EXCL _FEXCL
#define O_TRUNC _FTRUNC
#define O_APPEND _FAPPEND
#define O_NONBLOCK _FNONBLOCK

/* This tells Newlib's <fcntl.h> NOT to include <sys/_default_fcntl.h> */
#define _SYS__DEFAULT_FCNTL_H_

/* ... keep your struct flock and prototypes ... */

#ifdef __cplusplus
}
#endif
#endif
