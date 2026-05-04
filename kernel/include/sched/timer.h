#include "klib/stdint.h"
#include "klib/time.h"

#define TIMER_HZ 1000
#define TIMER_TICK_MS 1000 / TIMER_HZ

typedef uint64_t ticks_t;

void timer_on_tick(void *context);
ticks_t timer_tick_get();
void timer_tick_inc();
void timekeeping_get_walltime(timespec_t *tv);
