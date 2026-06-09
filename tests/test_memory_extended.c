/**
 * WebOS Extended Memory Management Tests
 *
 * Boundary conditions, edge cases, and stress tests for memory subsystem.
 */

#include "test_framework.h"

/* Skip WASM-specific host_func.h and provide stubs */
#define HOST_FUNC_H
#include <stdint.h>

static int32_t host_syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                                    int32_t a3, int32_t a4, int32_t a5) {
    (void)num; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return 0;
}

static void kernel_log(const char* msg) { (void)msg; }

#include "../kernel/src/memory.c"

/* ---- Tests ---- */

TEST(alloc_maximum_single_region) {
    memory_init();
    /* Allocate the largest reasonable single region */
    void* ptr = memory_alloc(stats.free_pages);
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(stats.free_pages, 0);
    return 0;
}

TEST(alloc_one_page) {
    memory_init();
    void* ptr = memory_alloc(1);
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(stats.used_pages, 5); /* 4 kernel + 1 alloc */
    return 0;
}

TEST(double_free_returns_error) {
    memory_init();
    void* ptr = memory_alloc(2);
    int32_t r1 = memory_free(ptr);
    ASSERT_EQ(r1, 0);
    /* Second free of the same pointer should fail - region is now FREE */
    int32_t r2 = memory_free(ptr);
    ASSERT_EQ(r2, -1);
    return 0;
}

TEST(free_nonexistent_region) {
    memory_init();
    /* Free an address that was never allocated */
    int32_t result = memory_free((void*)(uintptr_t)0xDEADBEEF);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(alloc_zero_pages_returns_null) {
    memory_init();
    void* ptr = memory_alloc(0);
    ASSERT_NULL(ptr);
    /* Stats should not change */
    ASSERT_EQ(stats.used_pages, 4);
    return 0;
}

TEST(sequential_alloc_free_pattern) {
    memory_init();
    uint32_t initial_used = stats.used_pages;

    /* Allocate 3 regions, then free them in order */
    void* p1 = memory_alloc(1);
    void* p2 = memory_alloc(2);
    void* p3 = memory_alloc(3);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    ASSERT_EQ(stats.used_pages, initial_used + 1 + 2 + 3);

    memory_free(p1);
    ASSERT_EQ(stats.used_pages, initial_used + 2 + 3);
    memory_free(p2);
    ASSERT_EQ(stats.used_pages, initial_used + 3);
    memory_free(p3);
    ASSERT_EQ(stats.used_pages, initial_used);
    return 0;
}

TEST(alloc_free_reverse_order) {
    memory_init();
    uint32_t initial_used = stats.used_pages;

    void* p1 = memory_alloc(1);
    void* p2 = memory_alloc(1);
    void* p3 = memory_alloc(1);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    /* Free in reverse order */
    memory_free(p3);
    memory_free(p2);
    memory_free(p1);
    ASSERT_EQ(stats.used_pages, initial_used);
    return 0;
}

TEST(alloc_after_free_reuses_slot) {
    memory_init();
    void* p1 = memory_alloc(1);
    memory_free(p1);
    /* After freeing, we should be able to allocate again */
    void* p2 = memory_alloc(1);
    ASSERT_NOT_NULL(p2);
    return 0;
}

TEST(memory_fragmentation_interleaved) {
    memory_init();
    /* Allocate 5 regions, free every other one to create fragmentation */
    void* p[5];
    for (int i = 0; i < 5; i++) {
        p[i] = memory_alloc(1);
        ASSERT_NOT_NULL(p[i]);
    }
    /* Free indices 1 and 3 */
    memory_free(p[1]);
    memory_free(p[3]);
    /* Should still be able to allocate new regions */
    void* q1 = memory_alloc(1);
    void* q2 = memory_alloc(1);
    ASSERT_NOT_NULL(q1);
    ASSERT_NOT_NULL(q2);
    return 0;
}

TEST(shared_memory_with_different_owners) {
    memory_init();
    void* s1 = memory_map_shared(65536, 1);  /* owner PID 1 */
    void* s2 = memory_map_shared(65536, 2);  /* owner PID 2 */
    void* s3 = memory_map_shared(65536, 42); /* owner PID 42 */
    ASSERT_NOT_NULL(s1);
    ASSERT_NOT_NULL(s2);
    ASSERT_NOT_NULL(s3);

    /* Verify owner PIDs */
    uint32_t a1 = (uint32_t)(uintptr_t)s1;
    uint32_t a2 = (uint32_t)(uintptr_t)s2;
    uint32_t a3 = (uint32_t)(uintptr_t)s3;
    int found1 = 0, found2 = 0, found3 = 0;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (regions[i].start == a1 && regions[i].type == MEM_REGION_SHARED) {
            ASSERT_EQ(regions[i].owner_pid, 1);
            found1 = 1;
        }
        if (regions[i].start == a2 && regions[i].type == MEM_REGION_SHARED) {
            ASSERT_EQ(regions[i].owner_pid, 2);
            found2 = 1;
        }
        if (regions[i].start == a3 && regions[i].type == MEM_REGION_SHARED) {
            ASSERT_EQ(regions[i].owner_pid, 42);
            found3 = 1;
        }
    }
    ASSERT_TRUE(found1);
    ASSERT_TRUE(found2);
    ASSERT_TRUE(found3);
    return 0;
}

TEST(shared_memory_increments_shared_pages) {
    memory_init();
    uint32_t before = stats.shared_pages;
    memory_map_shared(PAGE_SIZE * 3, 1);
    ASSERT_EQ(stats.shared_pages, before + 3);
    return 0;
}

TEST(stats_consistency_after_operations) {
    memory_init();
    /* total = used + free at all times */
    ASSERT_EQ(stats.total_pages, stats.used_pages + stats.free_pages);

    void* p1 = memory_alloc(5);
    ASSERT_EQ(stats.total_pages, stats.used_pages + stats.free_pages);

    void* p2 = memory_alloc(3);
    ASSERT_EQ(stats.total_pages, stats.used_pages + stats.free_pages);

    memory_free(p1);
    ASSERT_EQ(stats.total_pages, stats.used_pages + stats.free_pages);

    memory_free(p2);
    ASSERT_EQ(stats.total_pages, stats.used_pages + stats.free_pages);
    (void)p1; (void)p2;
    return 0;
}

TEST(region_overflow_max_regions) {
    memory_init();
    /* Fill all available region slots */
    int allocs = 0;
    for (int i = 0; i < MAX_REGIONS + 10; i++) {
        void* p = memory_alloc(1);
        if (p == NULL) break;
        allocs++;
    }
    /* Should have allocated up to MAX_REGIONS - 1 (one is kernel) */
    ASSERT_TRUE(allocs <= MAX_REGIONS - 1);
    ASSERT_TRUE(allocs > 0);
    return 0;
}

TEST(page_alignment_verification) {
    memory_init();
    void* p1 = memory_alloc(1);
    void* p2 = memory_alloc(1);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);

    /* All allocations should be page-aligned */
    uint32_t a1 = (uint32_t)(uintptr_t)p1;
    uint32_t a2 = (uint32_t)(uintptr_t)p2;
    ASSERT_EQ(a1 % PAGE_SIZE, 0);
    ASSERT_EQ(a2 % PAGE_SIZE, 0);
    return 0;
}

TEST(alloc_many_small_regions) {
    memory_init();
    void* ptrs[50];
    int count = 0;
    for (int i = 0; i < 50; i++) {
        ptrs[i] = memory_alloc(1);
        if (ptrs[i] != NULL) count++;
    }
    ASSERT_TRUE(count >= 10); /* Should allocate at least 10 */
    /* Free them all */
    for (int i = 0; i < count; i++) {
        memory_free(ptrs[i]);
    }
    return 0;
}

TEST(kernel_region_not_freeable) {
    memory_init();
    /* The kernel region starts at address 0 */
    int32_t result = memory_free((void*)(uintptr_t)0);
    /* Should fail because it's MEM_REGION_KERNEL, not MEM_REGION_USED */
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(shared_memory_alignment) {
    memory_init();
    /* Shared memory size not page-aligned should round up */
    void* ptr = memory_map_shared(100, 1); /* 100 bytes = 1 page */
    ASSERT_NOT_NULL(ptr);
    uint32_t addr = (uint32_t)(uintptr_t)ptr;
    ASSERT_EQ(addr % PAGE_SIZE, 0);
    return 0;
}

TEST(memory_stats_null_pointer_safe) {
    memory_init();
    /* Should not crash with NULL pointer */
    memory_stats(NULL);
    return 0;
}

TEST(multiple_init_resets_state) {
    memory_init();
    memory_alloc(5);
    memory_alloc(3);
    /* Re-init should reset everything */
    memory_init();
    ASSERT_EQ(stats.used_pages, 4); /* Just kernel */
    ASSERT_EQ(stats.kernel_pages, 4);
    return 0;
}

TEST(free_preserves_other_regions) {
    memory_init();
    void* p1 = memory_alloc(2);
    void* p2 = memory_alloc(3);
    void* p3 = memory_alloc(1);
    ASSERT_NOT_NULL(p1);
    ASSERT_NOT_NULL(p2);
    ASSERT_NOT_NULL(p3);

    /* Free p2, p1 and p3 should still be valid regions */
    memory_free(p2);
    /* Verify p1 and p3 still exist in regions */
    uint32_t a1 = (uint32_t)(uintptr_t)p1;
    uint32_t a3 = (uint32_t)(uintptr_t)p3;
    int found1 = 0, found3 = 0;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (regions[i].start == a1 && regions[i].type == MEM_REGION_USED) found1 = 1;
        if (regions[i].start == a3 && regions[i].type == MEM_REGION_USED) found3 = 1;
    }
    ASSERT_TRUE(found1);
    ASSERT_TRUE(found3);
    return 0;
}

static test_entry_t memory_extended_tests[] = {
    TEST_ENTRY(alloc_maximum_single_region, "Memory Extended"),
    TEST_ENTRY(alloc_one_page, "Memory Extended"),
    TEST_ENTRY(double_free_returns_error, "Memory Extended"),
    TEST_ENTRY(free_nonexistent_region, "Memory Extended"),
    TEST_ENTRY(alloc_zero_pages_returns_null, "Memory Extended"),
    TEST_ENTRY(sequential_alloc_free_pattern, "Memory Extended"),
    TEST_ENTRY(alloc_free_reverse_order, "Memory Extended"),
    TEST_ENTRY(alloc_after_free_reuses_slot, "Memory Extended"),
    TEST_ENTRY(memory_fragmentation_interleaved, "Memory Extended"),
    TEST_ENTRY(shared_memory_with_different_owners, "Memory Extended"),
    TEST_ENTRY(shared_memory_increments_shared_pages, "Memory Extended"),
    TEST_ENTRY(stats_consistency_after_operations, "Memory Extended"),
    TEST_ENTRY(region_overflow_max_regions, "Memory Extended"),
    TEST_ENTRY(page_alignment_verification, "Memory Extended"),
    TEST_ENTRY(alloc_many_small_regions, "Memory Extended"),
    TEST_ENTRY(kernel_region_not_freeable, "Memory Extended"),
    TEST_ENTRY(shared_memory_alignment, "Memory Extended"),
    TEST_ENTRY(memory_stats_null_pointer_safe, "Memory Extended"),
    TEST_ENTRY(multiple_init_resets_state, "Memory Extended"),
    TEST_ENTRY(free_preserves_other_regions, "Memory Extended"),
};

int main(void) {
    RUN_TESTS(memory_extended_tests);
}
