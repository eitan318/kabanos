#pragma once

#define NULL ((void *)0)

typedef unsigned int size_t;
typedef long ptrdiff_t;

#define SIZE_MAX ~((size_t)0)

// offsetof macro: finds the byte offset of a member in a struct
#define offsetof(type, member) ((size_t) & ((type *)0)->member)
