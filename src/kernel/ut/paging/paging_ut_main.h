#ifndef PAGING_UT_MAIN_H
#define PAGING_UT_MAIN_H

#include "../../frame_allocator/frame_allocator.h"

/**
 * Run all paging unit tests
 * 
 * @param allocator Pointer to initialized frame allocator
 */
void run_paging_tests(FrameAllocator* allocator);

#endif // PAGING_UT_MAIN_H