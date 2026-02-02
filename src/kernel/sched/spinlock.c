#include "hal.h"
#include "spinlock.h"

void spinlock_acquire(spinlock_t *lock) {
  int interrupts = hal_interrupts_state_get();

  __asm__ volatile("cli");
  while (__sync_bool_compare_and_swap(&lock->val, 0, 1) == 0) { }

  lock->interrupts = interrupts;
}

void spinlock_release(spinlock_t *lock) {
  while (__sync_bool_compare_and_swap(&lock->val, 1, 0) == 0) { }
  __asm__ volatile("sti");
}
