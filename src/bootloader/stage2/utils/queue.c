#include "queue.h"
#include <stddef.h>

void queue_init(Queue* q) { q->head = q->tail = q->count = 0; }

bool queue_is_empty(Queue* q) { return q->count == 0; }

bool queue_is_full(Queue* q) { return q->count == QUEUE_SIZE; }

void enqueue(Queue* q, void* val) {
    if (queue_is_full(q))
        return;
    q->data[q->head] = val;
    q->head = (q->head + 1) % QUEUE_SIZE;  // it was % q->count and made an error on some cases
    q->count++;
}

void* dequeue(Queue* q) {
    if (queue_is_empty(q))
        return NULL;
    void* val = q->data[q->tail];
    q->tail = (q->tail + 1) % QUEUE_SIZE;  // same as above
    q->count--;
    return val;
}