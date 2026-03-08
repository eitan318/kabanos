#pragma once
// The type that holds the information about variable arguments
typedef __builtin_va_list va_list;

// Start processing variable arguments
#define va_start(v, l) __builtin_va_start(v, l)

// Retrieve the next argument of a specific type
#define va_arg(v, l) __builtin_va_arg(v, l)

// Clean up the va_list
#define va_end(v) __builtin_va_end(v)

// Copy one va_list to another
#define va_copy(d, s) __builtin_va_copy(d, s)
