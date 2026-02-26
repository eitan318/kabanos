#include "sched/spinlock.h"
#include "hal.h"

void spinlock_acquire(spinlock_t *lock) {
  int interrupts = hal_interrupts_state_get();

  hal_interrupts_disable();
  while (__sync_bool_compare_and_swap(&lock->val, 0, 1) == 0) {
  }

  lock->interrupts = interrupts;
}

void spinlock_release(spinlock_t *lock) {
  int interrupts = lock->interrupts;
  while (__sync_bool_compare_and_swap(&lock->val, 1, 0) == 0) {
  }
  if (interrupts) {
    hal_interrupts_enable();
  }
}
