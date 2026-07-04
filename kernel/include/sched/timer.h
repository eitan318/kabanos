/**
 * @file timer.h
 * @brief System tick counter and wall-clock timekeeping.
 */
#include "klib/stdint.h"
#include "klib/time.h"

#define TIMER_HZ 1000
#define TIMER_TICK_MS 1000 / TIMER_HZ

typedef uint64_t ticks_t;

/** @brief Timer interrupt handler: advances the tick count, wakes
 *         sleepers and drives the scheduler. */
void timer_on_tick(void *context);

/** @brief Returns the number of ticks since boot. */
ticks_t timer_tick_get();

void timer_tick_inc();

/** @brief Returns the current wall-clock time derived from the tick
 *         count. */
void timekeeping_get_walltime(timespec_t *tv);
