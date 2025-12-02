#pragma once

#include "../../frame_allocator/frame_allocator.h"

/**
 * Run all paging unit tests
 * 
 * @param allocator Pointer to initialized frame allocator
 */
void paging_tests_run(FrameAllocator* allocator);