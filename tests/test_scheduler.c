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

#include "../kernel/src/process.c"
#include "../kernel/src/scheduler.c"

TEST(scheduler_init_defaults) {
    scheduler_init();
    ASSERT_EQ(sched_config.algorithm, SCHED_ROUND_ROBIN);
    ASSERT_EQ(sched_config.time_slice_ms, 100);
    return 0;
}

TEST(scheduler_yield_sets_ready) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    process_table[0].state = PROC_STATE_RUNNING;
    scheduler_yield();
    ASSERT_EQ(process_table[0].state, PROC_STATE_READY);
    return 0;
}

TEST(scheduler_block_sets_blocked) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    process_table[0].state = PROC_STATE_RUNNING;
    scheduler_block();
    ASSERT_EQ(process_table[0].state, PROC_STATE_BLOCKED);
    return 0;
}

TEST(scheduler_unblock_sets_ready) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    process_table[0].state = PROC_STATE_BLOCKED;
    scheduler_unblock(pid);
    ASSERT_EQ(process_table[0].state, PROC_STATE_READY);
    return 0;
}

TEST(scheduler_set_config) {
    scheduler_init();
    sched_config_t cfg = { .algorithm = 2, .time_slice_ms = 50 };
    int32_t result = scheduler_set_config(&cfg);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(sched_config.time_slice_ms, 50);
    return 0;
}

TEST(scheduler_set_config_null) {
    int32_t result = scheduler_set_config(NULL);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(scheduler_ready_count_empty) {
    process_init();
    scheduler_init();
    ASSERT_EQ(scheduler_ready_count(), 0);
    return 0;
}

TEST(scheduler_ready_count_with_processes) {
    process_init();
    scheduler_init();
    process_spawn("a.wex", PROC_PRIORITY_NORMAL);
    process_spawn("b.wex", PROC_PRIORITY_NORMAL);
    ASSERT_EQ(scheduler_ready_count(), 2);
    return 0;
}

static test_entry_t scheduler_tests[] = {
    TEST_ENTRY(scheduler_init_defaults, "Scheduler"),
    TEST_ENTRY(scheduler_yield_sets_ready, "Scheduler"),
    TEST_ENTRY(scheduler_block_sets_blocked, "Scheduler"),
    TEST_ENTRY(scheduler_unblock_sets_ready, "Scheduler"),
    TEST_ENTRY(scheduler_set_config, "Scheduler"),
    TEST_ENTRY(scheduler_set_config_null, "Scheduler"),
    TEST_ENTRY(scheduler_ready_count_empty, "Scheduler"),
    TEST_ENTRY(scheduler_ready_count_with_processes, "Scheduler"),
};

int main(void) {
    RUN_TESTS(scheduler_tests);
}
