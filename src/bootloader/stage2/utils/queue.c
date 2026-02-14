#include "queue.h"
#include <stddef.h>

void circular_buff_init(circular_buff_t *q) {
  q->head = q->tail = q->count = 0;
}

bool circular_buff_is_empty(circular_buff_t *q) { return q->count == 0; }

bool circular_buff_is_full(circular_buff_t *q) {
  return q->count == QUEUE_SIZE;
}

void circular_buff_enqueue(circular_buff_t *q, void *val) {
  if (circular_buff_is_full(q))
    return;
  q->data[q->head] = val;
  q->head = (q->head + 1) %
            QUEUE_SIZE; // it was % q->count and made an error on some cases
  q->count++;
}

void *circular_buff_dequeue(circular_buff_t *q) {
  if (circular_buff_is_empty(q))
    return NULL;
  void *val = q->data[q->tail];
  q->tail = (q->tail + 1) % QUEUE_SIZE; // same as above
  q->count--;
  return val;
}
