#ifndef _SYS_TIME_H_
#define _SYS_TIME_H_

#include <sys/types.h> // This lets Newlib define time_t correctly first

// Only define things Newlib hasn't defined yet
typedef struct {
  time_t tv_sec;
  long tv_nsec;
} timespec_t;

#endif
