/**
 * @file circular_buffer.h
 * @brief Fixed-capacity circular FIFO of pointers.
 */
#ifndef QUEUE_H
#define QUEUE_H

#include "klib/stdbool.h"

#define QUEUE_SIZE 256

/** @brief Ring buffer holding up to QUEUE_SIZE void pointers. */
typedef struct {
  void *data[QUEUE_SIZE]; /**< Element storage. */
  int head;               /**< Next write (enqueue) index. */
  int tail;               /**< Next read (dequeue) index. */
  int count;              /**< Current number of elements. */
} circular_buff_t;

/** @brief Resets the buffer to the empty state. */
void circular_buff_init(circular_buff_t *q);

bool circular_buff_is_empty(circular_buff_t *q);

bool circular_buff_is_full(circular_buff_t *q);

/** @brief Appends @p val; drops it silently if the buffer is full. */
void circular_buff_enqueue(circular_buff_t *q, void *val);

/** @brief Removes and returns the oldest element, or NULL if empty. */
void *circular_buff_dequeue(circular_buff_t *q);

/**
 * @brief Discards the most recently enqueued element (e.g. backspace
 *        handling in line editing).
 * @return true if an element was removed.
 */
bool circular_buff_dequeue_last(circular_buff_t *cb);

#endif
