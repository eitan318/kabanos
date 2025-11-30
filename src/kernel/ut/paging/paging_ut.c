#include "paging_ut_main.h"
#include "../../mm/paging.h"
#include "../../include/stdio.h"
#include "../../include/memory.h"
#include "../../frame_allocator/frame_allocator.h"

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

static void page_directory_create_test(void) {
    test_start_print("Create Page Directory");
    
    PageDirectoryT* pd = page_directory_create();
    
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    debugf("  Page directory pointer: 0x%x\n", (uint32_t)pd);
    debugf("  Checking entries...\n");
    
    // Check all 1024 entries
    int non_zero_count = 0;
    for (int i = 0; i < PAGE_DIRECTORY_ENTRIES; i++) {
        if (pd->entries[i].present != 0 || pd->entries[i].frame != 0) {
            non_zero_count++;
        }
    }
    
    debugf("  Total entries checked: %d\n", PAGE_DIRECTORY_ENTRIES);
    debugf("  Non-zero entries found: %d\n", non_zero_count);
    
    if (non_zero_count > 0) {
        page_directory_destroy(pd);
        fail_print("Found non-zero entries");
        return;
    }
    
    page_directory_destroy(pd);
    pass_print();
}

static void page_directory_destroy_test(void) {
    test_start_print("Destroy Page Directory");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    uint32_t pd_addr = (uint32_t)pd;
    debugf("  Created page directory at: 0x%x\n", pd_addr);
    debugf("  Destroying...\n");
    
    page_directory_destroy(pd);
    debugf("  Destroyed successfully\n");
    pass_print();
}

static void page_directory_entry_set_test(void) {
    test_start_print("Set Page Directory Entry");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    uint32_t page_table_addr = 0x00400000;
    uint32_t flags = PDE_PRESENT | PDE_WRITE | PDE_USER;
    
    debugf("  Setting entry 0:\n");
    debugf("    Physical address: 0x%x\n", page_table_addr);
    debugf("    Flags: PRESENT | WRITE | USER\n");
    
    page_directory_entry_set(pd, 0, page_table_addr, flags);
    
    uint32_t expected_frame = page_table_addr >> 12;
    
    debugf("  Verifying entry:\n");
    debugf("    Present: %d (expected 1)\n", pd->entries[0].present);
    debugf("    Write:   %d (expected 1)\n", pd->entries[0].write);
    debugf("    User:    %d (expected 1)\n", pd->entries[0].user);
    debugf("    Frame:   0x%x (expected 0x%x)\n", pd->entries[0].frame, expected_frame);
    
    bool success = (pd->entries[0].present == 1 && 
                   pd->entries[0].write == 1 && 
                   pd->entries[0].user == 1 && 
                   pd->entries[0].frame == expected_frame);
    
    page_directory_destroy(pd);
    
    if (success) {
        pass_print();
    } else {
        fail_print("Entry values incorrect");
    }
}

static void page_table_entry_set_test(void) {
    test_start_print("Set Page Table Entry");
    
    PageTableT* pt = page_table_create();
    if (pt == NULL) {
        fail_print("Page table is NULL");
        return;
    }
    
    debugf("  Page table created at: 0x%x\n", (uint32_t)pt);
    
    uint32_t physical_addr = 0x00500000;
    uint32_t flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
    
    debugf("  Setting entry 0:\n");
    debugf("    Physical address: 0x%x\n", physical_addr);
    debugf("    Flags: PRESENT | WRITE | USER\n");
    
    page_table_entry_set(pt, 0, physical_addr, flags);
    
    uint32_t expected_frame = physical_addr >> 12;
    
    debugf("  Verifying entry:\n");
    debugf("    Present: %d (expected 1)\n", pt->entries[0].present);
    debugf("    Write:   %d (expected 1)\n", pt->entries[0].write);
    debugf("    User:    %d (expected 1)\n", pt->entries[0].user);
    debugf("    Frame:   0x%x (expected 0x%x)\n", pt->entries[0].frame, expected_frame);
    
    bool success = (pt->entries[0].present == 1 && 
                   pt->entries[0].write == 1 && 
                   pt->entries[0].user == 1 && 
                   pt->entries[0].frame == expected_frame);
    
    page_table_destroy(pt);
    
    if (success) {
        pass_print();
    } else {
        fail_print("Entry values incorrect");
    }
}

static void page_directories_multiple_test(void) {
    test_start_print("Multiple Page Directories");
    
    debugf("  Creating 3 page directories...\n");
    PageDirectoryT* pd1 = page_directory_create();
    PageDirectoryT* pd2 = page_directory_create();
    PageDirectoryT* pd3 = page_directory_create();
    
    if (pd1 == NULL || pd2 == NULL || pd3 == NULL) {
        fail_print("Could not create all directories");
        if (pd1) page_directory_destroy(pd1);
        if (pd2) page_directory_destroy(pd2);
        if (pd3) page_directory_destroy(pd3);
        return;
    }
    
    debugf("  Directory addresses:\n");
    debugf("    pd1: 0x%x\n", (uint32_t)pd1);
    debugf("    pd2: 0x%x\n", (uint32_t)pd2);
    debugf("    pd3: 0x%x\n", (uint32_t)pd3);
    
    // Check uniqueness
    bool unique = !((uint32_t)pd1 == (uint32_t)pd2 || 
                   (uint32_t)pd2 == (uint32_t)pd3 || 
                   (uint32_t)pd1 == (uint32_t)pd3);
    
    if (unique) {
        debugf("  All addresses are unique\n");
    } else {
        debugf("  ERROR: Duplicate addresses found\n");
    }
    
    // Check alignment
    bool aligned = ((uint32_t)pd1 % PAGE_SIZE == 0 &&
                   (uint32_t)pd2 % PAGE_SIZE == 0 &&
                   (uint32_t)pd3 % PAGE_SIZE == 0);
    
    if (aligned) {
        debugf("  All directories are 4KB aligned\n");
    } else {
        debugf("  WARNING: Some directories not 4KB aligned\n");
    }
    
    debugf("  Destroying directories (pd2, pd1, pd3)...\n");
    page_directory_destroy(pd2);
    page_directory_destroy(pd1);
    page_directory_destroy(pd3);
    debugf("  All directories destroyed\n");
    
    if (unique && aligned) {
        pass_print();
    } else {
        fail_print("Directories not unique or not aligned");
    }
}

static void page_directory_lifecycle_test(void) {
    test_start_print("Page Directory Lifecycle");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    debugf("  Page directory created at: 0x%x\n", (uint32_t)pd);
    debugf("  Setting 3 entries with different flags...\n\n");
    
    // Set entries
    page_directory_entry_set(pd, 0, 0x00400000, PDE_PRESENT | PDE_WRITE);
    page_directory_entry_set(pd, 1, 0x00401000, PDE_PRESENT | PDE_WRITE | PDE_USER);
    page_directory_entry_set(pd, 2, 0x00402000, PDE_PRESENT);
    
    debugf("  Entry configurations:\n");
    debugf("    [0] addr=0x%x P=%d W=%d U=%d\n", 
           pd->entries[0].frame << 12,
           pd->entries[0].present, pd->entries[0].write, pd->entries[0].user);
    debugf("    [1] addr=0x%x P=%d W=%d U=%d\n",
           pd->entries[1].frame << 12,
           pd->entries[1].present, pd->entries[1].write, pd->entries[1].user);
    debugf("    [2] addr=0x%x P=%d W=%d U=%d\n",
           pd->entries[2].frame << 12,
           pd->entries[2].present, pd->entries[2].write, pd->entries[2].user);
    debugf("    [3] addr=0x%x P=%d W=%d U=%d (should be 0)\n",
           pd->entries[3].frame << 12,
           pd->entries[3].present, pd->entries[3].write, pd->entries[3].user);
    
    bool success = (pd->entries[0].present == 1 && pd->entries[0].write == 1 && pd->entries[0].user == 0 &&
                   pd->entries[1].present == 1 && pd->entries[1].write == 1 && pd->entries[1].user == 1 &&
                   pd->entries[2].present == 1 && pd->entries[2].write == 0 && pd->entries[2].user == 0 &&
                   pd->entries[3].present == 0);
    
    page_directory_destroy(pd);
    
    if (success) {
        pass_print();
    } else {
        fail_print("Some entries configured incorrectly");
    }
}

static void page_mapping_test(void) {
    test_start_print("Page Mapping");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    uint32_t virt_addr = 0xC0000000;
    uint32_t phys_addr = 0x00100000;
    
    debugf("  Mapping virtual to physical:\n");
    debugf("    Virtual:  0x%x (3GB)\n", virt_addr);
    debugf("    Physical: 0x%x (1MB)\n", phys_addr);
    debugf("    Flags:    PRESENT | WRITE\n");
    
    bool result = paging_page_map(pd, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITE);
    
    if (!result) {
        fail_print("paging_page_map returned false");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Mapping operation successful\n");
    
    // Verify
    uint32_t retrieved = paging_physical_address_get(pd, virt_addr);
    debugf("  Verification:\n");
    debugf("    Retrieved: 0x%x\n", retrieved);
    debugf("    Expected:  0x%x\n", phys_addr);
    
    if (retrieved != phys_addr) {
        fail_print("Physical address mismatch");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("  Address verified successfully\n");
    
    page_directory_destroy(pd);
    pass_print();
}

static void page_unmapping_test(void) {
    test_start_print("Page Unmapping");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    uint32_t virt_addr = 0xC0001000;
    uint32_t phys_addr = 0x00200000;
    
    debugf("  Step 1: Map page\n");
    debugf("    Virtual:  0x%x\n", virt_addr);
    debugf("    Physical: 0x%x\n", phys_addr);
    
    if (!paging_page_map(pd, virt_addr, phys_addr, PTE_PRESENT | PTE_WRITE)) {
        fail_print("Could not map page");
        page_directory_destroy(pd);
        return;
    }
    
    uint32_t retrieved = paging_physical_address_get(pd, virt_addr);
    if (retrieved != phys_addr) {
        fail_print("Initial mapping verification failed");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("    Mapping verified\n");
    
    debugf("  Step 2: Unmap page\n");
    if (!paging_page_unmap(pd, virt_addr)) {
        fail_print("paging_page_unmap returned false");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("    Unmap operation completed\n");
    
    debugf("  Step 3: Verify unmapping\n");
    retrieved = paging_physical_address_get(pd, virt_addr);
    debugf("    Retrieved: 0x%x (expected 0x0)\n", retrieved);
    
    if (retrieved != 0) {
        fail_print("Page still mapped after unmapping");
        page_directory_destroy(pd);
        return;
    }
    
    debugf("    Unmapping verified\n");
    
    page_directory_destroy(pd);
    pass_print();
}

static void paging_state_test(void) {
    test_start_print("Paging State");
    
    bool enabled = paging_is_enabled();
    debugf("  Current paging state: %s\n", enabled ? "ENABLED" : "DISABLED");
    
    if (enabled) {
        debugf("  WARNING: Paging already enabled at test start\n");
    } else {
        debugf("  Paging is correctly disabled\n");
    }
    
    pass_print();
}

static void multiple_page_mappings_test(void) {
    test_start_print("Multiple Page Mappings");
    
    PageDirectoryT* pd = page_directory_create();
    if (pd == NULL) {
        fail_print("Page directory is NULL");
        return;
    }
    
    debugf("  Mapping 5 consecutive pages...\n");
    
    uint32_t virt_base = 0xC0000000;
    uint32_t phys_base = 0x00100000;
    
    // Map pages
    for (int i = 0; i < 5; i++) {
        uint32_t virt = virt_base + (i * PAGE_SIZE);
        uint32_t phys = phys_base + (i * PAGE_SIZE);
        
        if (!paging_page_map(pd, virt, phys, PTE_PRESENT | PTE_WRITE)) {
            debugf("    ERROR: Failed to map page %d\n", i);
            fail_print("Could not map all pages");
            page_directory_destroy(pd);
            return;
        }
    }
    
    debugf("  All 5 pages mapped successfully\n\n");
    debugf("  Verifying all mappings:\n");
    
    bool all_correct = true;
    for (int i = 0; i < 5; i++) {
        uint32_t virt = virt_base + (i * PAGE_SIZE);
        uint32_t expected = phys_base + (i * PAGE_SIZE);
        uint32_t actual = paging_physical_address_get(pd, virt);
        
        debugf("    [%d] 0x%x -> 0x%x ", i, virt, actual);
        
        if (actual == expected) {
            debugf("[OK]\n");
        } else {
            debugf("[FAIL - expected 0x%x]\n", expected);
            all_correct = false;
        }
    }
    
    page_directory_destroy(pd);
    
    if (all_correct) {
        pass_print();
    } else {
        fail_print("Some mappings incorrect");
    }
}

void paging_tests_run(FrameAllocator* allocator) {
    header_print("PAGING UNIT TESTS");
    
    if (allocator == NULL) {
        debugf("\nERROR: NULL allocator provided\n");
        return;
    }
    
    debugf("\nFrame Allocator Status:\n");
    debugf("  Total frames: %llu\n", frame_get_total_count(allocator));
    debugf("  Free frames:  %llu\n", frame_get_free_count(allocator));
    debugf("  Used frames:  %llu\n", frame_get_used_count(allocator));
    
    debugf("\nInitializing paging system...\n");
    paging_init(allocator);
    debugf("Paging system initialized\n");
    
    // Run all tests
    page_directory_create_test();
    page_directory_destroy_test();
    page_directory_entry_set_test();
    page_table_entry_set_test();
    page_directories_multiple_test();
    page_directory_lifecycle_test();
    page_mapping_test();
    page_unmapping_test();
    paging_state_test();
    multiple_page_mappings_test();
    
    header_print("ALL TESTS COMPLETE");
    debugf("\n");
}