#pragma once

#define SPINLOCK_RELEASED {.val=0, .interrupts=0};
#define SPINLOCK_ACQUIRED {.val=1, .interrupts=0};

typedef struct spinlock {
  volatile unsigned val;
  volatile unsigned interrupts;
} spinlock_t;

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
