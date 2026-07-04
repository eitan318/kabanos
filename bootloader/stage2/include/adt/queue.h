/**
 * @file queue.h
 * @brief Fixed-capacity circular FIFO of pointers.
 */
#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#define QUEUE_SIZE 256

typedef struct {
  void *data[QUEUE_SIZE]; // the buffer
  int head;               // index to write (enqueue)
  int tail;               // index to read (dequeue)
  int count;              // current number of elements
} circular_buff_t;

void circular_buff_init(circular_buff_t *q);

bool circular_buff_is_empty(circular_buff_t *q);

bool circular_buff_is_full(circular_buff_t *q);

void circular_buff_enqueue(circular_buff_t *q, void *val);

void *circular_buff_dequeue(circular_buff_t *q);

#endif
