#pragma once

// Unsigned integers
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

// Signed integers
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned int uintptr_t;
typedef int intptr_t;

#define UINT_MAX(x) ~((x)0)
#define UINT64_MAX UINT_MAX(uint64_t)
