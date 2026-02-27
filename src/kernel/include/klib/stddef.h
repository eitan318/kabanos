#pragma once

#define NULL ((void *)0)

// size_t is unsigned, used for counts and sizes
#ifdef __x86_64__
typedef unsigned long long size_t;
#else
typedef unsigned int size_t;
#endif

// ptrdiff_t is signed, used for the result of subtracting two pointers
typedef long ptrdiff_t;

// offsetof macro: finds the byte offset of a member in a struct
#define offsetof(type, member) ((size_t) & ((type *)0)->member)
