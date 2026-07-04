/**
 * @file spinlock.h
 * @brief Busy-waiting mutual-exclusion lock.
 *
 * Acquiring a spinlock also disables local interrupts; the previous
 * interrupt state is saved in the lock and restored on release.
 */
#pragma once

/** @brief Static initializer for an unlocked spinlock. */
#define SPINLOCK_RELEASED {.val=0, .interrupts=0};
/** @brief Static initializer for a lock that starts out held. */
#define SPINLOCK_ACQUIRED {.val=1, .interrupts=0};

typedef struct spinlock {
  volatile unsigned val;        /**< 1 while held, 0 when free. */
  volatile unsigned interrupts; /**< Interrupt state saved at acquire time. */
} spinlock_t;

/** @brief Spins until the lock is acquired; disables local interrupts. */
void spinlock_acquire(spinlock_t *lock);

/** @brief Releases the lock and restores the saved interrupt state. */
void spinlock_release(spinlock_t *lock);
