#include "memory.h"

void* memset(void* ptr, int value, size_t num) {
    uint8_t* u8_ptr = (uint8_t*)ptr;

    for (size_t i = 0; i < num; i++)
        u8_ptr[i] = (uint8_t)value;

    return ptr;
}

// Kernel heap location - place it after kernel end
// We'll use a fixed location that's definitely safe
#define HEAP_START 0x400000  // 4MB mark - well after kernel and modules
#define HEAP_SIZE  (1024 * 1024)  // 1MB heap

// DO NOT initialize these - let them go in BSS
static uint8_t* heap_start;
static size_t heap_size;
static size_t heap_used;
static bool heap_initialized;

// Initialize the heap - call this BEFORE using kmalloc
void kmalloc_init(void) {
    if (heap_initialized) {
        return;  // Already initialized
    }
    
    heap_start = (uint8_t*)HEAP_START;
    heap_size = HEAP_SIZE;
    heap_used = 0;
    heap_initialized = true;
}

void* kmalloc(size_t size) {
    // Auto-initialize on first use (but should be called explicitly)
    if (!heap_initialized) {
        kmalloc_init();
    }
    
    if (size == 0) {
        return NULL;
    }
    
    // Align size to 16 bytes
    size = (size + 15) & ~15;
    
    if (heap_used + size > heap_size) {
        return NULL;  // Out of memory
    }
    
    void* ptr = heap_start + heap_used;
    heap_used += size;
    
    return ptr;
}

void kfree(void* ptr) {
    // Simple bump allocator doesn't support free
    (void)ptr;
}

// Debug function to check heap usage
size_t kmalloc_used(void) {
    return heap_used;
}

size_t kmalloc_available(void) {
    return heap_size - heap_used;
}