#include "klib/time.h"
#include "sched/timer.h"

long sys_gettimeofday(timespec_t *tv, void *tz) {
  if (!tv)
    return -1;
  timekeeping_get_walltime(tv);
  return 0;
}
