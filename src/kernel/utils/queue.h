#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

#define QUEUE_SIZE 256

typedef struct {
    void* data[QUEUE_SIZE]; // the buffer
    int head;               // index to write (enqueue)
    int tail;               // index to read (dequeue)
    int count;              // current number of elements
} Queue;

void queue_init(Queue* q);

bool queue_is_empty(Queue* q);

bool queue_is_full(Queue* q);

void enqueue(Queue* q, void* val);

void* dequeue(Queue* q);

#endif
