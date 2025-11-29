#include "paging_ut_main.h"
#include "../../mm/paging.h"
#include "../../include/stdio.h"
#include "../../include/memory.h"
#include "../../frame_allocator/frame_allocator.h"

static void test_create_page_directory(void) {
    printf("\n=== Test: Create Page Directory ===\n");
    
    page_directory_t* pd = create_page_directory();
    
    if (pd == NULL) {
        printf("FAIL: Page directory is NULL\n");
        return;
    }
    
    printf("Page directory pointer: 0x%x\n", (uint32_t)pd);
    printf("Checking if entries are zeroed...\n");
    
    // Check first 5 entries in detail
    bool all_zero = true;
    for (int i = 0; i < 5; i++) {
        printf("  Entry[%d]: present=%d, write=%d, user=%d, frame=0x%x\n",
               i,
               pd->entries[i].present,
               pd->entries[i].write,
               pd->entries[i].user,
               pd->entries[i].frame);
        
        if (pd->entries[i].present != 0 || pd->entries[i].frame != 0) {
            all_zero = false;
        }
    }
    
    // Check all 1024 entries
    int non_zero_count = 0;
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        if (pd->entries[i].present != 0 || pd->entries[i].frame != 0) {
            non_zero_count++;
        }
    }
    
    printf("Total entries checked: %d\n", PAGE_DIRECTORY_ENTRIES);
    printf("Non-zero entries found: %d\n", non_zero_count);
    
    if (non_zero_count > 0) {
        printf("FAIL: Found %d non-zero entries\n", non_zero_count);
        destroy_page_directory(pd);
        return;
    }
    
    destroy_page_directory(pd);
    printf("PASS: All %d entries are properly zeroed\n", PAGE_DIRECTORY_ENTRIES);
}

static void test_destroy_page_directory(void) {
    printf("\n=== Test: Destroy Page Directory ===\n");
    
    page_directory_t* pd = create_page_directory();
    if (pd == NULL) {
        printf("FAIL: Page directory is NULL\n");
        return;
    }
    
    uint32_t pd_addr = (uint32_t)pd;
    printf("Created page directory at: 0x%x\n", pd_addr);
    printf("Destroying page directory...\n");
    
    destroy_page_directory(pd);
    printf("PASS: Page directory destroyed (no crash)\n");
}

static void test_set_page_directory_entry(void) {
    printf("\n=== Test: Set Page Directory Entry ===\n");
    
    page_directory_t* pd = create_page_directory();
    if (pd == NULL) {
        printf("FAIL: Page directory is NULL\n");
        return;
    }
    
    uint32_t page_table_addr = 0x00400000;
    uint32_t flags = PDE_PRESENT | PDE_WRITE | PDE_USER;
    
    printf("Setting entry 0 with:\n");
    printf("  Physical address: 0x%x\n", page_table_addr);
    printf("  Flags: 0x%x (PRESENT=%d, WRITE=%d, USER=%d)\n", 
           flags,
           (flags & PDE_PRESENT) ? 1 : 0,
           (flags & PDE_WRITE) ? 1 : 0,
           (flags & PDE_USER) ? 1 : 0);
    
    printf("BEFORE: Entry[0]: present=%d, write=%d, user=%d, frame=0x%x\n",
           pd->entries[0].present,
           pd->entries[0].write,
           pd->entries[0].user,
           pd->entries[0].frame);
    
    set_page_directory_entry(pd, 0, page_table_addr, flags);
    
    printf("AFTER:  Entry[0]: present=%d, write=%d, user=%d, frame=0x%x\n",
           pd->entries[0].present,
           pd->entries[0].write,
           pd->entries[0].user,
           pd->entries[0].frame);
    
    uint32_t expected_frame = page_table_addr >> 12;
    printf("Expected frame number: 0x%x\n", expected_frame);
    printf("Actual frame number:   0x%x\n", pd->entries[0].frame);
    
    bool success = true;
    if (pd->entries[0].present != 1) {
        printf("ERROR: present flag is %d, expected 1\n", pd->entries[0].present);
        success = false;
    }
    if (pd->entries[0].write != 1) {
        printf("ERROR: write flag is %d, expected 1\n", pd->entries[0].write);
        success = false;
    }
    if (pd->entries[0].user != 1) {
        printf("ERROR: user flag is %d, expected 1\n", pd->entries[0].user);
        success = false;
    }
    if (pd->entries[0].frame != expected_frame) {
        printf("ERROR: frame is 0x%x, expected 0x%x\n", pd->entries[0].frame, expected_frame);
        success = false;
    }
    
    // Reconstruct physical address from entry
    uint32_t reconstructed_addr = pd->entries[0].frame << 12;
    printf("Reconstructed physical address: 0x%x\n", reconstructed_addr);
    
    if (reconstructed_addr != page_table_addr) {
        printf("ERROR: Reconstructed address doesn't match original!\n");
        success = false;
    }
    
    destroy_page_directory(pd);
    
    if (success) {
        printf("PASS: Page directory entry set correctly\n");
    } else {
        printf("FAIL: Page directory entry has errors\n");
    }
}

static void test_set_page_table_entry(void) {
    printf("\n=== Test: Set Page Table Entry ===\n");
    printf("SKIP: Page table entry test (requires 4KB allocation)\n");
}

static void test_multiple_directories(void) {
    printf("\n=== Test: Multiple Page Directories ===\n");
    
    printf("Creating 3 page directories...\n");
    page_directory_t* pd1 = create_page_directory();
    page_directory_t* pd2 = create_page_directory();
    page_directory_t* pd3 = create_page_directory();
    
    if (pd1 == NULL || pd2 == NULL || pd3 == NULL) {
        printf("FAIL: Could not create all directories\n");
        printf("  pd1: 0x%x\n", (uint32_t)pd1);
        printf("  pd2: 0x%x\n", (uint32_t)pd2);
        printf("  pd3: 0x%x\n", (uint32_t)pd3);
        if (pd1) destroy_page_directory(pd1);
        if (pd2) destroy_page_directory(pd2);
        if (pd3) destroy_page_directory(pd3);
        return;
    }
    
    printf("Directory addresses:\n");
    printf("  pd1: 0x%x\n", (uint32_t)pd1);
    printf("  pd2: 0x%x\n", (uint32_t)pd2);
    printf("  pd3: 0x%x\n", (uint32_t)pd3);
    
    // Check uniqueness
    bool unique = true;
    if ((uint32_t)pd1 == (uint32_t)pd2) {
        printf("ERROR: pd1 and pd2 have same address!\n");
        unique = false;
    }
    if ((uint32_t)pd2 == (uint32_t)pd3) {
        printf("ERROR: pd2 and pd3 have same address!\n");
        unique = false;
    }
    if ((uint32_t)pd1 == (uint32_t)pd3) {
        printf("ERROR: pd1 and pd3 have same address!\n");
        unique = false;
    }
    
    // Check alignment (should be 4KB aligned)
    if ((uint32_t)pd1 % PAGE_SIZE != 0) {
        printf("WARNING: pd1 not 4KB aligned (addr=0x%x)\n", (uint32_t)pd1);
    }
    if ((uint32_t)pd2 % PAGE_SIZE != 0) {
        printf("WARNING: pd2 not 4KB aligned (addr=0x%x)\n", (uint32_t)pd2);
    }
    if ((uint32_t)pd3 % PAGE_SIZE != 0) {
        printf("WARNING: pd3 not 4KB aligned (addr=0x%x)\n", (uint32_t)pd3);
    }
    
    printf("Destroying directories in order: pd2, pd1, pd3...\n");
    destroy_page_directory(pd2);
    printf("  pd2 destroyed\n");
    destroy_page_directory(pd1);
    printf("  pd1 destroyed\n");
    destroy_page_directory(pd3);
    printf("  pd3 destroyed\n");
    
    if (unique) {
        printf("PASS: All directories unique and properly managed\n");
    } else {
        printf("FAIL: Directory addresses not unique\n");
    }
}

static void test_page_directory_lifecycle(void) {
    printf("\n=== Test: Page Directory Lifecycle ===\n");
    
    page_directory_t* pd = create_page_directory();
    if (pd == NULL) {
        printf("FAIL: Page directory is NULL\n");
        return;
    }
    
    printf("Page directory created at: 0x%x\n", (uint32_t)pd);
    printf("Setting 3 entries with different addresses and flags...\n");
    
    // Set 3 entries with different configurations
    set_page_directory_entry(pd, 0, 0x00400000, PDE_PRESENT | PDE_WRITE);
    printf("Entry[0] set: addr=0x00400000, flags=PRESENT|WRITE\n");
    
    set_page_directory_entry(pd, 1, 0x00401000, PDE_PRESENT | PDE_WRITE | PDE_USER);
    printf("Entry[1] set: addr=0x00401000, flags=PRESENT|WRITE|USER\n");
    
    set_page_directory_entry(pd, 2, 0x00402000, PDE_PRESENT);
    printf("Entry[2] set: addr=0x00402000, flags=PRESENT\n");
    
    printf("\nVerifying entries:\n");
    
    // Verify entry 0
    printf("Entry[0]: present=%d, write=%d, user=%d, frame=0x%x (addr=0x%x)\n",
           pd->entries[0].present,
           pd->entries[0].write,
           pd->entries[0].user,
           pd->entries[0].frame,
           pd->entries[0].frame << 12);
    
    // Verify entry 1
    printf("Entry[1]: present=%d, write=%d, user=%d, frame=0x%x (addr=0x%x)\n",
           pd->entries[1].present,
           pd->entries[1].write,
           pd->entries[1].user,
           pd->entries[1].frame,
           pd->entries[1].frame << 12);
    
    // Verify entry 2
    printf("Entry[2]: present=%d, write=%d, user=%d, frame=0x%x (addr=0x%x)\n",
           pd->entries[2].present,
           pd->entries[2].write,
           pd->entries[2].user,
           pd->entries[2].frame,
           pd->entries[2].frame << 12);
    
    // Verify entry 3 is still zero
    printf("Entry[3]: present=%d, write=%d, user=%d, frame=0x%x (should be all 0)\n",
           pd->entries[3].present,
           pd->entries[3].write,
           pd->entries[3].user,
           pd->entries[3].frame);
    
    bool success = true;
    
    // Check entry 0
    if (pd->entries[0].present != 1 || pd->entries[0].write != 1 || pd->entries[0].user != 0) {
        printf("ERROR: Entry[0] flags incorrect\n");
        success = false;
    }
    if (pd->entries[0].frame != (0x00400000 >> 12)) {
        printf("ERROR: Entry[0] frame incorrect\n");
        success = false;
    }
    
    // Check entry 1
    if (pd->entries[1].present != 1 || pd->entries[1].write != 1 || pd->entries[1].user != 1) {
        printf("ERROR: Entry[1] flags incorrect\n");
        success = false;
    }
    if (pd->entries[1].frame != (0x00401000 >> 12)) {
        printf("ERROR: Entry[1] frame incorrect\n");
        success = false;
    }
    
    // Check entry 2
    if (pd->entries[2].present != 1 || pd->entries[2].write != 0 || pd->entries[2].user != 0) {
        printf("ERROR: Entry[2] flags incorrect\n");
        success = false;
    }
    if (pd->entries[2].frame != (0x00402000 >> 12)) {
        printf("ERROR: Entry[2] frame incorrect\n");
        success = false;
    }
    
    // Check entry 3 is unchanged
    if (pd->entries[3].present != 0 || pd->entries[3].frame != 0) {
        printf("ERROR: Entry[3] was modified\n");
        success = false;
    }
    
    destroy_page_directory(pd);
    
    if (success) {
        printf("PASS: All entries set correctly with proper flags\n");
    } else {
        printf("FAIL: Some entries have errors\n");
    }
}

void run_paging_tests(FrameAllocator* allocator) {
    printf("\n");
    printf("====================================\n");
    printf("========== PAGING TESTS ===========\n");
    printf("====================================\n");
    
    if (allocator == NULL) {
        printf("ERROR: NULL allocator provided\n");
        return;
    }
    
    printf("Frame allocator status:\n");
    printf("  Total frames: %llu\n", frame_get_total_count(allocator));
    printf("  Free frames:  %llu\n", frame_get_free_count(allocator));
    printf("  Used frames:  %llu\n", frame_get_used_count(allocator));
    
    printf("\nInitializing paging system...\n");
    paging_init(allocator);
    
    test_create_page_directory();
    test_destroy_page_directory();
    test_set_page_directory_entry();
    test_set_page_table_entry();
    test_multiple_directories();
    test_page_directory_lifecycle();
    
    printf("\n");
    printf("====================================\n");
    printf("===== PAGING TESTS COMPLETE =======\n");
    printf("====================================\n");
    printf("\n");
}