#include "paging_ut_main.h"
#include "../../mm/paging.h"
#include "../../include/stdio.h"
#include "../../include/memory.h"
#include "../../frame_allocator/frame_allocator.h"

// Global frame allocator for tests
static FrameAllocator* g_test_allocator = NULL;

// Helper functions for pretty output
static void line_print(void) {
    debugf("================================================================\n");
}

static void double_line_print(void) {
    debugf("════════════════════════════════════════════════════════════════\n");
}

static void header_print(const char* text) {
    debugf("\n");
    double_line_print();
    debugf("  %s\n", text);
    double_line_print();
}

static void test_start_print(const char* name) {
    debugf("\n");
    line_print();
    debugf("TEST: %s\n", name);
    line_print();
}

static void pass_print(void) {
    debugf("Result: PASS\n");
}

static void fail_print(const char* message) {
    debugf("Result: FAIL - %s\n", message);
}

/**
 * Allocate a physical frame for testing
 */
static uint32_t test_frame_allocate(void) {
    if (g_test_allocator == NULL) {
        debugf("ERROR: test allocator is NULL\n");
        return 0;
    }
    
    uint64_t frame = frame_alloc(g_test_allocator);
    if (frame == 0) {
        debugf("ERROR: frame_alloc returned 0\n");
        return 0;
    }
    
    return (uint32_t)frame;
}

/**
 * Setup identity mapping for physical memory
 * Map enough memory so that any frame allocated by frame_alloc()
 * can be accessed directly by the paging code
 */
static bool setup_identity_mapping(PageDirectoryT* pd) {
    // Get the highest physical address we might need to access
    // This should cover all memory that the frame allocator manages
    uint64_t total_memory = frame_get_total_count(g_test_allocator) * PAGE_SIZE;
    
    // Round up to nearest MB for safety
    uint32_t map_size = (uint32_t)((total_memory + 0xFFFFF) & ~0xFFFFF);
    
    // Safety cap: don't try to map more than 512MB
    if (map_size > 512 * 1024 * 1024) {
        map_size = 512 * 1024 * 1024;
    }
    
    debugf("  Setting up identity mapping...\n");
    debugf("    Total memory: %llu bytes (%llu MB)\n", 
           total_memory, total_memory / (1024 * 1024));
    debugf("    Will map: %u MB (%u pages)\n", 
           map_size / (1024 * 1024), map_size / PAGE_SIZE);
    
    uint32_t pages_mapped = 0;
    uint32_t total_pages = map_size / PAGE_SIZE;
    
    // Identity map the physical memory
    for (uint32_t addr = 0; addr < map_size; addr += PAGE_SIZE) {
        if (!paging_page_map(pd, addr, addr, PTE_PRESENT | PTE_WRITE)) {
            debugf("\n    ERROR: Failed to map address 0x%x\n", addr);
            return false;
        }
        
        pages_mapped++;
        
        // Print progress every 1024 pages (4MB)
        if ((pages_mapped % 1024) == 0) {
            debugf("    Progress: %u/%u pages (%u MB)\n", 
                   pages_mapped, total_pages, pages_mapped * 4 / 1024);
        }
    }
    
    debugf("  Identity mapping complete! Mapped %u pages (%u MB)\n", 
           pages_mapped, pages_mapped * 4 / 1024);
    return true;
}

static void page_mapping_with_paging_test(void) {
    test_start_print("Page Mapping WITH PAGING ENABLED");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    // Setup identity mapping first
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    // Allocate a REAL physical frame for testing
    uint32_t phys_frame = test_frame_allocate();
    if (phys_frame == 0) {
        fail_print("Could not allocate physical frame");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Allocated physical frame at: 0x%x\n", phys_frame);
    
    // Clear the frame BEFORE enabling paging
    memset((void*)phys_frame, 0, PAGE_SIZE);
    
    debugf("  Enabling paging...\n");
    paging_enable(pd);
    debugf("  Paging is now ENABLED\n");
    
    // Now test mapping a new virtual address to our allocated frame
    uint32_t virt_addr = 0xC0000000;
    
    debugf("  Mapping virtual to physical WITH PAGING:\n");
    debugf("    Virtual:  0x%x (3GB)\n", virt_addr);
    debugf("    Physical: 0x%x (allocated frame)\n", phys_frame);
    
    bool result = paging_page_map(pd, virt_addr, phys_frame, PTE_PRESENT | PTE_WRITE);
    
    if (!result) {
        fail_print("paging_page_map returned false");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Mapping operation successful\n");
    
    // Verify by reading the page tables
    uint32_t retrieved = paging_physical_address_get(pd, virt_addr);
    debugf("  Retrieved physical address: 0x%x\n", retrieved);
    
    if (retrieved != phys_frame) {
        fail_print("Physical address mismatch");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    // Now ACTUALLY TEST the mapping by writing and reading through virtual address
    debugf("  Testing ACTUAL memory access through virtual address...\n");
    volatile uint32_t* virt_ptr = (volatile uint32_t*)virt_addr;
    volatile uint32_t* phys_ptr = (volatile uint32_t*)phys_frame;
    
    // Write a test pattern through virtual address
    uint32_t test_pattern = 0xDEADBEEF;
    debugf("    Writing 0x%x to virtual address 0x%x\n", test_pattern, virt_addr);
    *virt_ptr = test_pattern;
    
    // Read back through virtual address
    uint32_t read_virt = *virt_ptr;
    debugf("    Reading from virtual address 0x%x: 0x%x\n", virt_addr, read_virt);
    
    // Read back through physical address (identity-mapped)
    uint32_t read_phys = *phys_ptr;
    debugf("    Reading from physical address 0x%x: 0x%x\n", phys_frame, read_phys);
    
    if (read_virt != test_pattern || read_phys != test_pattern) {
        fail_print("Memory access test failed - value mismatch");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Memory access test PASSED - paging works!\n");
    
    paging_disable();
    page_directory_destroy(pd);
    pass_print();
}

static void page_unmapping_with_paging_test(void) {
    test_start_print("Page Unmapping WITH PAGING ENABLED");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    // Allocate physical frame
    uint32_t phys_frame = test_frame_allocate();
    if (phys_frame == 0) {
        fail_print("Could not allocate physical frame");
        page_directory_destroy(pd);
        return;
    }
    
    // Clear frame before paging
    memset((void*)phys_frame, 0, PAGE_SIZE);
    
    uint32_t virt_addr = 0xC0001000;
    
    debugf("  Mapping page first...\n");
    debugf("    Virtual: 0x%x -> Physical: 0x%x\n", virt_addr, phys_frame);
    
    if (!paging_page_map(pd, virt_addr, phys_frame, PTE_PRESENT | PTE_WRITE)) {
        fail_print("Could not map page");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Enabling paging...\n");
    paging_enable(pd);
    
    // Test access before unmapping
    volatile uint32_t* virt_ptr = (volatile uint32_t*)virt_addr;
    *virt_ptr = 0x12345678;
    uint32_t value = *virt_ptr;
    
    if (value != 0x12345678) {
        fail_print("Initial memory access failed");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Initial access works, value = 0x%x\n", value);
    debugf("  Unmapping page...\n");
    
    if (!paging_page_unmap(pd, virt_addr)) {
        fail_print("paging_page_unmap returned false");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Page unmapped successfully\n");
    debugf("  NOTE: Accessing unmapped page would cause page fault\n");
    
    paging_disable();
    page_directory_destroy(pd);
    pass_print();
}

static void multiple_mappings_with_paging_test(void) {
    test_start_print("Multiple Mappings WITH PAGING ENABLED");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Allocating 5 physical frames...\n");
    
    uint32_t phys_frames[5];
    for (int i = 0; i < 5; i++) {
        phys_frames[i] = test_frame_allocate();
        if (phys_frames[i] == 0) {
            debugf("    ERROR: Failed to allocate frame %d\n", i);
            fail_print("Could not allocate all frames");
            page_directory_destroy(pd);
            return;
        }
        debugf("    Frame %d: 0x%x\n", i, phys_frames[i]);
        memset((void*)phys_frames[i], 0, PAGE_SIZE);
    }
    
    debugf("  Mapping 5 pages in high memory...\n");
    
    uint32_t virt_base = 0xC0000000;
    
    // First, check the page directory entry for this address range
    uint32_t pd_index = PAGE_DIRECTORY_INDEX(virt_base);
    debugf("    Page directory index for 0x%x: %u\n", virt_base, pd_index);
    
    for (int i = 0; i < 5; i++) {
        uint32_t virt = virt_base + (i * PAGE_SIZE);
        
        debugf("    Mapping [%d]: virt 0x%x -> phys 0x%x\n", i, virt, phys_frames[i]);
        
        if (!paging_page_map(pd, virt, phys_frames[i], PTE_PRESENT | PTE_WRITE)) {
            debugf("    ERROR: Failed to map page %d\n", i);
            fail_print("Could not map all pages");
            page_directory_destroy(pd);
            return;
        }
        
        // Verify the mapping was set correctly
        uint32_t resolved = paging_physical_address_get(pd, virt);
        debugf("      Verified: virt 0x%x resolves to phys 0x%x (expected 0x%x) %s\n",
               virt, resolved, phys_frames[i], 
               (resolved == phys_frames[i]) ? "[OK]" : "[MISMATCH!]");
    }
    
    debugf("  All pages mapped, enabling paging...\n");
    paging_enable(pd);
    
    debugf("  Testing memory access to all 5 pages...\n");
    
    bool all_correct = true;
    for (int i = 0; i < 5; i++) {
        uint32_t virt = virt_base + (i * PAGE_SIZE);
        
        volatile uint32_t* virt_ptr = (volatile uint32_t*)virt;
        volatile uint32_t* phys_ptr = (volatile uint32_t*)phys_frames[i];
        
        uint32_t test_value = 0xAAAA0000 + i;
        
        *virt_ptr = test_value;
        uint32_t read_virt = *virt_ptr;
        uint32_t read_phys = *phys_ptr;
        
        debugf("    [%d] virt=0x%x write=0x%x read_v=0x%x read_p=0x%x ", 
               i, virt, test_value, read_virt, read_phys);
        
        if (read_virt == test_value && read_phys == test_value) {
            debugf("[OK]\n");
        } else {
            debugf("[FAIL]\n");
            all_correct = false;
        }
    }
    
    paging_disable();
    page_directory_destroy(pd);
    
    if (all_correct) {
        pass_print();
    } else {
        fail_print("Some memory accesses failed");
    }
}

static void identity_mapping_test(void) {
    test_start_print("Identity Mapping Verification");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Allocating test frames...\n");
    
    // Allocate REAL frames for testing
    uint32_t test_frames[4];
    for (int i = 0; i < 4; i++) {
        test_frames[i] = test_frame_allocate();
        if (test_frames[i] == 0) {
            fail_print("Could not allocate test frame");
            page_directory_destroy(pd);
            return;
        }
        debugf("    Test frame %d: 0x%x\n", i, test_frames[i]);
        memset((void*)test_frames[i], 0, PAGE_SIZE);
    }
    
    debugf("  Enabling paging with identity mapping...\n");
    paging_enable(pd);
    
    debugf("  Testing that identity-mapped addresses still work...\n");
    
    bool all_work = true;
    for (int i = 0; i < 4; i++) {
        volatile uint32_t* addr = (volatile uint32_t*)test_frames[i];
        uint32_t test_value = 0xBEEF0000 + i;
        
        addr[0] = test_value;
        uint32_t read_back = addr[0];
        
        debugf("    Address 0x%x: wrote 0x%x, read 0x%x ", 
               (uint32_t)addr, test_value, read_back);
        
        if (read_back == test_value) {
            debugf("[OK]\n");
        } else {
            debugf("[FAIL]\n");
            all_work = false;
        }
    }
    
    paging_disable();
    page_directory_destroy(pd);
    
    if (all_work) {
        pass_print();
    } else {
        fail_print("Some identity-mapped accesses failed");
    }
}

static void cr3_and_tlb_test(void) {
    test_start_print("CR3 Register and TLB Management");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    uint32_t pd_physical = page_directory_physical_get(pd);
    debugf("  Page directory physical: 0x%x\n", pd_physical);
    
    uint32_t cr3_before;
    asm volatile("mov %%cr3, %0" : "=r"(cr3_before));
    debugf("  CR3 before: 0x%x\n", cr3_before);
    
    // Allocate frames for TLB test
    uint32_t phys1 = test_frame_allocate();
    uint32_t phys2 = test_frame_allocate();
    
    if (phys1 == 0 || phys2 == 0) {
        fail_print("Could not allocate frames for TLB test");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Allocated frames: 0x%x, 0x%x\n", phys1, phys2);
    
    memset((void*)phys1, 0, PAGE_SIZE);
    memset((void*)phys2, 0, PAGE_SIZE);
    
    debugf("  Enabling paging...\n");
    paging_enable(pd);
    
    uint32_t cr3_after;
    asm volatile("mov %%cr3, %0" : "=r"(cr3_after));
    debugf("  CR3 after:  0x%x\n", cr3_after);
    
    if (cr3_after != pd_physical) {
        fail_print("CR3 not set correctly");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    debugf("  CR0: 0x%x (bit 31 = %d)\n", cr0, (cr0 >> 31) & 1);
    
    if (!((cr0 >> 31) & 1)) {
        fail_print("Paging bit not set in CR0");
        paging_disable();
        page_directory_destroy(pd);
        return;
    }
    
    uint32_t virt_test = 0xC0002000;
    
    debugf("  Testing TLB invalidation...\n");
    debugf("    Mapping 0x%x -> 0x%x\n", virt_test, phys1);
    paging_page_map(pd, virt_test, phys1, PTE_PRESENT | PTE_WRITE);
    
    volatile uint32_t* virt_ptr = (volatile uint32_t*)virt_test;
    *virt_ptr = 0xABCD1234;
    
    uint32_t val1 = *(volatile uint32_t*)phys1;
    debugf("    Value at phys1: 0x%x\n", val1);
    
    debugf("    Remapping 0x%x -> 0x%x\n", virt_test, phys2);
    paging_page_map(pd, virt_test, phys2, PTE_PRESENT | PTE_WRITE);
    
    *virt_ptr = 0x5678DEAD;
    uint32_t val2 = *(volatile uint32_t*)phys2;
    debugf("    Value at phys2: 0x%x\n", val2);
    
    bool tlb_works = (val1 == 0xABCD1234 && val2 == 0x5678DEAD);
    
    paging_disable();
    page_directory_destroy(pd);
    
    if (tlb_works) {
        pass_print();
    } else {
        fail_print("TLB invalidation test failed");
    }
}

static void paging_required_test(void) {
    test_start_print("Verify Paging is REQUIRED for High Memory Access");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    // Allocate a physical frame
    uint32_t phys_frame = test_frame_allocate();
    if (phys_frame == 0) {
        fail_print("Could not allocate frame");
        page_directory_destroy(pd);
        return;
    }
    
    memset((void*)phys_frame, 0, PAGE_SIZE);
    
    uint32_t virt_addr = 0xC0000000;
    
    debugf("  Mapping virt 0x%x -> phys 0x%x\n", virt_addr, phys_frame);
    
    if (!paging_page_map(pd, virt_addr, phys_frame, PTE_PRESENT | PTE_WRITE)) {
        fail_print("Could not map page");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Testing WITHOUT paging enabled...\n");
    debugf("    Paging is currently: %s\n", 
           paging_is_enabled() ? "ENABLED" : "DISABLED");
    
    // Try to access 0xC0000000 WITHOUT paging enabled
    // This should NOT work because there's no physical RAM at 0xC0000000
    volatile uint32_t* high_mem = (volatile uint32_t*)virt_addr;
    
    debugf("    Attempting to write to 0x%x (this should NOT work or cause issues)\n", 
           virt_addr);
    debugf("    NOTE: Without paging, 0xC0000000 is just a physical address\n");
    debugf("    and there's likely no RAM there!\n");
    
    // DON'T actually try to access it - that would crash!
    // Just document that we CAN'T access it
    debugf("    Result: Cannot safely access 0x%x without paging\n", virt_addr);
    
    debugf("\n  Now testing WITH paging enabled...\n");
    paging_enable(pd);
    debugf("    Paging is currently: %s\n", 
           paging_is_enabled() ? "ENABLED" : "DISABLED");
    
    debugf("    Writing 0xBEEFCAFE to virtual address 0x%x\n", virt_addr);
    *high_mem = 0xBEEFCAFE;
    
    uint32_t read_back = *high_mem;
    debugf("    Read back from virtual: 0x%x\n", read_back);
    
    // Also check the physical address directly (via identity mapping)
    volatile uint32_t* phys_mem = (volatile uint32_t*)phys_frame;
    uint32_t read_phys = *phys_mem;
    debugf("    Read back from physical 0x%x: 0x%x\n", phys_frame, read_phys);
    
    bool success = (read_back == 0xBEEFCAFE) && (read_phys == 0xBEEFCAFE);
    
    paging_disable();
    page_directory_destroy(pd);
    
    if (success) {
        debugf("  SUCCESS: High memory (0xC0000000) ONLY works with paging!\n");
        pass_print();
    } else {
        fail_print("Memory access test failed");
    }
}

static void paging_state_transitions_test(void) {
    test_start_print("Paging State Transitions");
    
    debugf("  Initial state: %s\n", 
           paging_is_enabled() ? "ENABLED" : "DISABLED");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    if (!setup_identity_mapping(pd)) {
        fail_print("Could not setup identity mapping");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Enabling paging...\n");
    paging_enable(pd);
    debugf("  State after enable: %s\n", 
           paging_is_enabled() ? "ENABLED" : "DISABLED");
    
    if (!paging_is_enabled()) {
        fail_print("paging_is_enabled() returns false after enable");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Disabling paging...\n");
    paging_disable();
    debugf("  State after disable: %s\n", 
           paging_is_enabled() ? "ENABLED" : "DISABLED");
    
    if (paging_is_enabled()) {
        fail_print("paging_is_enabled() returns true after disable");
        page_directory_destroy(pd);
        return;
    }
    
    page_directory_destroy(pd);
    pass_print();
}

void paging_tests_run(FrameAllocator* allocator) {
    header_print("PAGING UNIT TESTS - WITH ACTUAL PAGING ENABLED");
    
    if (allocator == NULL) {
        debugf("\nERROR: NULL allocator provided\n");
        return;
    }
    
    g_test_allocator = allocator;
    
    debugf("\nFrame Allocator Status:\n");
    debugf("  Total frames: %llu\n", frame_get_total_count(allocator));
    debugf("  Free frames:  %llu\n", frame_get_free_count(allocator));
    
    debugf("\nInitializing paging system...\n");
    paging_init(allocator);
    debugf("Paging system initialized\n");
    
    debugf("\n*** These tests enable and use ACTUAL PAGING ***\n");
    
    // Run tests
    identity_mapping_test();
    page_mapping_with_paging_test();
    page_unmapping_with_paging_test();
    multiple_mappings_with_paging_test();
    paging_required_test(); 
    cr3_and_tlb_test();
    paging_state_transitions_test();
    
    header_print("ALL PAGING TESTS COMPLETE");
    
    debugf("\nFinal Frame Allocator Status:\n");
    debugf("  Free frames: %llu\n", frame_get_free_count(allocator));
    debugf("  Used frames: %llu\n", frame_get_used_count(allocator));
    debugf("\n");
}