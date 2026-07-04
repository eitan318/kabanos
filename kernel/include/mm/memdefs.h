/**
 * @file memdefs.h
 * @brief Fixed virtual/physical memory layout of the kernel.
 *
 * The kernel is higher-half: all of physical memory is mapped at
 * KERNEL_BASE, so kernel virtual = physical + KERNEL_BASE.
 */
#pragma once

/* --- Physical --- */
#define VGA_SCREEN_BUF_PHYS 0xB8000

/* --- Kernel virtual --- */
#define KERNEL_BASE 0xC0000000

#define VGA_SCREEN_BUF (KERNEL_BASE + VGA_SCREEN_BUF_PHYS)

/** @brief Largest amount of physical RAM the fixed mapping supports. */
#define MAX_PHYSICAL_MEMORY 0x20000000 // 512 MB

#define BOOT_STACK_SIZE 16384

/* Kernel heap sits right above the physical-memory mapping */
#define KERNEL_HEAP_START (KERNEL_BASE + MAX_PHYSICAL_MEMORY)
#define KERNEL_HEAP_SIZE 0x10000000 // 256 MB
#define KERNEL_HEAP_END (KERNEL_HEAP_START + KERNEL_HEAP_SIZE)

/** @brief Scratch window for temporarily mapping one arbitrary
 *         physical page. */
#define KMAPPING_BASE (KERNEL_HEAP_END + 0x00001000)
#define KMAPPING_SIZE PAGE_SIZE
#define KMAPPING_END (KMAPPING_BASE + KMAPPING_SIZE)

/* Leave a safety gap between the heap and the per-process kernel stacks */
#define PROCESS_KERNEL_STACKS_START (KMAPPING_END + 0x01000000)
#define PROCESS_KERNEL_STACK_SIZE 0x10000

/* --- Process virtual --- */
#define USER_STACK_TOP 0xBFFFF000 /**< Just below kernel space (3GB). */
#define USER_STACK_SIZE 0x10000
#define USER_STACK_BOTTOM (USER_STACK_TOP - USER_STACK_SIZE)

#define USER_HEAP_START 0x20000000
#define USER_HEAP_INITIAL PAGE_SIZE /**< One page to start. */
