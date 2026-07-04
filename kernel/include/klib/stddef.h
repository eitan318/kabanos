/**
 * @file stddef.h
 * @brief Core scalar types and NULL (from compiler builtins).
 */
#pragma once

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __INTPTR_TYPE__ intptr_t;

#define SIZE_MAX ~((size_t)0)

#define NULL ((void *)0)
#define offsetof(type, member) __builtin_offsetof(type, member)
