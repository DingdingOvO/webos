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

/* We include the kernel source directly for unit testing */
#include "../kernel/src/memory.c"

TEST(memory_init_returns_zero) {
    int32_t result = memory_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(memory_init_sets_kernel_region) {
    memory_init();
    ASSERT_EQ(regions[0].type, MEM_REGION_KERNEL);
    ASSERT_EQ(regions[0].start, 0);
    ASSERT_TRUE(regions[0].size > 0);
    return 0;
}

TEST(memory_init_sets_stats) {
    memory_init();
    ASSERT_TRUE(stats.total_pages > 0);
    ASSERT_TRUE(stats.used_pages > 0);
    return 0;
}

TEST(memory_alloc_returns_non_null) {
    memory_init();
    void* ptr = memory_alloc(1);
    ASSERT_NOT_NULL(ptr);
    return 0;
}

TEST(memory_alloc_increments_used_pages) {
    memory_init();
    uint32_t before = stats.used_pages;
    memory_alloc(4);
    ASSERT_EQ(stats.used_pages, before + 4);
    return 0;
}

TEST(memory_alloc_decrements_free_pages) {
    memory_init();
    uint32_t before = stats.free_pages;
    memory_alloc(2);
    ASSERT_EQ(stats.free_pages, before - 2);
    return 0;
}

TEST(memory_alloc_zero_pages_returns_null) {
    memory_init();
    void* ptr = memory_alloc(0);
    ASSERT_NULL(ptr);
    return 0;
}

TEST(memory_free_returns_zero_on_success) {
    memory_init();
    void* ptr = memory_alloc(2);
    int32_t result = memory_free(ptr);
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(memory_free_null_returns_error) {
    memory_init();
    int32_t result = memory_free(NULL);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(memory_free_updates_stats) {
    memory_init();
    uint32_t before_used = stats.used_pages;
    void* ptr = memory_alloc(3);
    memory_free(ptr);
    ASSERT_EQ(stats.used_pages, before_used);
    return 0;
}

TEST(memory_alloc_multiple_regions) {
    memory_init();
    void* ptr1 = memory_alloc(2);
    void* ptr2 = memory_alloc(3);
    ASSERT_NOT_NULL(ptr1);
    ASSERT_NOT_NULL(ptr2);
    ASSERT_TRUE((uint32_t)(uintptr_t)ptr2 >= (uint32_t)(uintptr_t)ptr1 + 2 * PAGE_SIZE);
    return 0;
}

TEST(memory_map_shared) {
    memory_init();
    void* ptr = memory_map_shared(8192, 5);
    ASSERT_NOT_NULL(ptr);
    /* Find the region and verify it's shared */
    uint32_t addr = (uint32_t)(uintptr_t)ptr;
    int found = 0;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (regions[i].start == addr && regions[i].type == MEM_REGION_SHARED) {
            ASSERT_EQ(regions[i].owner_pid, 5);
            found = 1;
            break;
        }
    }
    ASSERT_TRUE(found);
    return 0;
}

TEST(memory_stats_returns_data) {
    memory_init();
    mem_stats_t s;
    memory_stats(&s);
    ASSERT_EQ(s.total_pages, stats.total_pages);
    ASSERT_EQ(s.used_pages, stats.used_pages);
    return 0;
}

static test_entry_t memory_tests[] = {
    TEST_ENTRY(memory_init_returns_zero, "Memory"),
    TEST_ENTRY(memory_init_sets_kernel_region, "Memory"),
    TEST_ENTRY(memory_init_sets_stats, "Memory"),
    TEST_ENTRY(memory_alloc_returns_non_null, "Memory"),
    TEST_ENTRY(memory_alloc_increments_used_pages, "Memory"),
    TEST_ENTRY(memory_alloc_decrements_free_pages, "Memory"),
    TEST_ENTRY(memory_alloc_zero_pages_returns_null, "Memory"),
    TEST_ENTRY(memory_free_returns_zero_on_success, "Memory"),
    TEST_ENTRY(memory_free_null_returns_error, "Memory"),
    TEST_ENTRY(memory_free_updates_stats, "Memory"),
    TEST_ENTRY(memory_alloc_multiple_regions, "Memory"),
    TEST_ENTRY(memory_map_shared, "Memory"),
    TEST_ENTRY(memory_stats_returns_data, "Memory"),
};

int main(void) {
    RUN_TESTS(memory_tests);
}
