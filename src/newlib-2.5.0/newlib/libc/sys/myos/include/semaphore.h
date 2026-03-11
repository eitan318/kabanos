#pragma once
typedef int sem_t;
static inline int sem_init(sem_t *s, int p, unsigned v) {
  (void)s;
  (void)p;
  (void)v;
  return 0;
}
static inline int sem_wait(sem_t *s) {
  (void)s;
  return 0;
}
static inline int sem_post(sem_t *s) {
  (void)s;
  return 0;
}
static inline int sem_destroy(sem_t *s) {
  (void)s;
  return 0;
}
