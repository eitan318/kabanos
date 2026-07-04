/**
 * @file spinlock.c
 * @brief Spinlock implementation using GCC atomic builtins.
 *
 * Interrupts are disabled for the whole time a lock is held so that an
 * interrupt handler on the same CPU cannot deadlock against the holder.
 */
#include "spinlock.h"
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
