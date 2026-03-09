#include "sched/timer.h"
#include "assert.h"
#include "hal.h"
#include "klib/time.h"
#include "modules.h"
#include "sched/sched.h"

uint64_t g_cpu_loops_per_ns;

static uint32_t g_time_tick = 0;

int time_init(module_t *self) {
  //  g_cpu_loops_per_ns = current_cpu_measure_loops_per_ns();
  hal_timer_init(TIMER_HZ);
  return 0;
}

ticks_t timer_tick_get() { return g_time_tick; }

void timer_on_tick(void *context) {
  g_time_tick++;
  sched_on_timer_tick(context);
}

static timespec_t boot_time_offset = {0, 0};

void timekeeping_get_walltime(timespec_t *tv) {
  ASSERT(tv);

  uint64_t ticks = timer_tick_get();

  // 1. Calculate seconds and nanoseconds from ticks
  // Assuming TIMER_TICK_MS is 1 (1ms per tick)
  uint64_t total_ms = ticks * TIMER_TICK_MS;

  uint64_t seconds = total_ms / 1000;
  uint64_t nanoseconds = (total_ms % 1000) * 1000000;

  // 2. Add the boot time offset (Wall clock time)
  tv->tv_sec = boot_time_offset.tv_sec + seconds;
  tv->tv_nsec = boot_time_offset.tv_nsec + nanoseconds;

  // 3. Handle nanosecond overflow
  if (tv->tv_nsec >= 1000000000L) {
    tv->tv_sec++;
    tv->tv_nsec -= 1000000000L;
  }
}

static const char *timer_deps[] = {"hal", NULL};

ITER_MODULE(timer) = {
    .name = "timer",
    .required_modules_names = timer_deps,
    .init = &time_init,
    .fini = NULL,
};
