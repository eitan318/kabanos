#pragma once

typedef long time_t;

struct tm {
  int tm_sec;   /* 0-60 */
  int tm_min;   /* 0-59 */
  int tm_hour;  /* 0-23 */
  int tm_mday;  /* 1-31 */
  int tm_mon;   /* 0-11 */
  int tm_year;  /* years since 1900 */
  int tm_wday;  /* 0-6, Sunday=0 */
  int tm_yday;  /* 0-365 */
  int tm_isdst; /* daylight saving flag */
};

time_t time(time_t *tloc);
struct tm *localtime(const time_t *timep);
