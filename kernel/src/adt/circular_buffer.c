#include "adt/circular_buffer.h"
#include "klib/stddef.h"

void circular_buff_init(circular_buff_t *cb) {
  cb->head = cb->tail = cb->count = 0;
}

bool circular_buff_is_empty(circular_buff_t *cb) { return cb->count == 0; }

bool circular_buff_is_full(circular_buff_t *cb) {
  return cb->count == QUEUE_SIZE;
}

void circular_buff_enqueue(circular_buff_t *cb, void *val) {
  if (circular_buff_is_full(cb))
    return;
  cb->data[cb->head] = val;
  cb->head = (cb->head + 1) %
             QUEUE_SIZE; // it was % q->count and made an error on some cases
  cb->count++;
}

void *circular_buff_dequeue(circular_buff_t *cb) {
  if (circular_buff_is_empty(cb))
    return NULL;
  void *val = cb->data[cb->tail];
  cb->tail = (cb->tail + 1) % QUEUE_SIZE; // same as above
  cb->count--;
  return val;
}

bool circular_buff_dequeue_last(circular_buff_t *cb) {
  if (cb->count == 0)
    return false;

  // Move the HEAD back, because that's where the last inserted char is
  if (cb->head == 0) {
    cb->head = QUEUE_SIZE - 1;
  } else {
    cb->head = cb->head - 1;
  }

  cb->count--;
  return true;
}
