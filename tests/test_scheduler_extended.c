/**
 * WebOS Extended Scheduler Tests
 *
 * Round-robin simulation, priority effects, blocking/unblocking, and edge cases.
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

#include "../kernel/src/process.c"
#include "../kernel/src/scheduler.c"

/* ---- Tests ---- */

TEST(scheduler_init_resets_config) {
    /* Set some config first */
    sched_config_t cfg = { .algorithm = SCHED_PRIORITY, .time_slice_ms = 50 };
    scheduler_set_config(&cfg);

    /* Re-init should reset */
    scheduler_init();
    ASSERT_EQ(sched_config.algorithm, SCHED_ROUND_ROBIN);
    ASSERT_EQ(sched_config.time_slice_ms, 100);
    return 0;
}

TEST(scheduler_yield_from_ready_state) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    /* Process is in READY state (not RUNNING), yield should be no-op */
    scheduler_yield();
    process_t* proc = process_get(pid);
    ASSERT_EQ(proc->state, PROC_STATE_READY); /* No change */
    return 0;
}

TEST(scheduler_block_from_ready_state) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    /* Process is in READY state (not RUNNING), block should be no-op */
    scheduler_block();
    process_t* proc = process_get(pid);
    ASSERT_EQ(proc->state, PROC_STATE_READY); /* No change */
    return 0;
}

TEST(scheduler_unblock_non_blocked_process) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    /* Process is in READY state, not BLOCKED - unblock should be no-op */
    scheduler_unblock(pid);
    process_t* proc = process_get(pid);
    ASSERT_EQ(proc->state, PROC_STATE_READY); /* No change */
    return 0;
}

TEST(scheduler_round_trip_running_ready_running) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    process_t* proc = process_get(pid);

    /* READY -> RUNNING */
    proc->state = PROC_STATE_RUNNING;
    ASSERT_EQ(proc->state, PROC_STATE_RUNNING);

    /* RUNNING -> READY (yield) */
    scheduler_yield();
    ASSERT_EQ(proc->state, PROC_STATE_READY);

    /* READY -> RUNNING again */
    proc->state = PROC_STATE_RUNNING;
    ASSERT_EQ(proc->state, PROC_STATE_RUNNING);
    return 0;
}

TEST(scheduler_round_trip_running_blocked_ready) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    process_t* proc = process_get(pid);

    /* READY -> RUNNING */
    proc->state = PROC_STATE_RUNNING;
    /* RUNNING -> BLOCKED */
    scheduler_block();
    ASSERT_EQ(proc->state, PROC_STATE_BLOCKED);

    /* BLOCKED -> READY */
    scheduler_unblock(pid);
    ASSERT_EQ(proc->state, PROC_STATE_READY);
    return 0;
}

TEST(scheduler_multiple_yield_cycles) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    process_t* proc = process_get(pid);

    /* Simulate multiple scheduling cycles */
    for (int i = 0; i < 5; i++) {
        proc->state = PROC_STATE_RUNNING;
        scheduler_yield();
        ASSERT_EQ(proc->state, PROC_STATE_READY);
    }
    return 0;
}

TEST(scheduler_multiple_block_unblock_cycles) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    process_t* proc = process_get(pid);

    for (int i = 0; i < 5; i++) {
        current_pid = pid;
        proc->state = PROC_STATE_RUNNING;
        scheduler_block();
        ASSERT_EQ(proc->state, PROC_STATE_BLOCKED);
        scheduler_unblock(pid);
        ASSERT_EQ(proc->state, PROC_STATE_READY);
    }
    return 0;
}

TEST(scheduler_ready_count_with_blocked) {
    process_init();
    scheduler_init();
    pid_t p1 = process_spawn("a.wex", PROC_PRIORITY_NORMAL);
    pid_t p2 = process_spawn("b.wex", PROC_PRIORITY_NORMAL);
    pid_t p3 = process_spawn("c.wex", PROC_PRIORITY_NORMAL);

    ASSERT_EQ(scheduler_ready_count(), 3);

    /* Block one process */
    process_get(p2)->state = PROC_STATE_BLOCKED;
    ASSERT_EQ(scheduler_ready_count(), 2);

    /* Block another */
    process_get(p1)->state = PROC_STATE_BLOCKED;
    ASSERT_EQ(scheduler_ready_count(), 1);
    return 0;
}

TEST(scheduler_ready_count_with_zombie) {
    process_init();
    scheduler_init();
    process_spawn("a.wex", PROC_PRIORITY_NORMAL);
    pid_t p2 = process_spawn("b.wex", PROC_PRIORITY_NORMAL);
    process_spawn("c.wex", PROC_PRIORITY_NORMAL);

    ASSERT_EQ(scheduler_ready_count(), 3);

    /* Kill one process (zombie) */
    process_kill(p2);
    ASSERT_EQ(scheduler_ready_count(), 2);
    return 0;
}

TEST(scheduler_set_config_priority) {
    scheduler_init();
    sched_config_t cfg = { .algorithm = SCHED_PRIORITY, .time_slice_ms = 25 };
    int32_t result = scheduler_set_config(&cfg);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(sched_config.algorithm, SCHED_PRIORITY);
    ASSERT_EQ(sched_config.time_slice_ms, 25);
    return 0;
}

TEST(scheduler_get_config) {
    scheduler_init();
    sched_config_t cfg;
    scheduler_get_config(&cfg);
    ASSERT_EQ(cfg.algorithm, SCHED_ROUND_ROBIN);
    ASSERT_EQ(cfg.time_slice_ms, 100);
    return 0;
}

TEST(scheduler_get_config_null) {
    scheduler_init();
    /* Should not crash */
    scheduler_get_config(NULL);
    return 0;
}

TEST(scheduler_config_change_during_operation) {
    process_init();
    scheduler_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;

    /* Change config while processes exist */
    sched_config_t cfg = { .algorithm = SCHED_PRIORITY, .time_slice_ms = 50 };
    scheduler_set_config(&cfg);

    /* Operations should still work */
    process_get(pid)->state = PROC_STATE_RUNNING;
    scheduler_yield();
    ASSERT_EQ(process_get(pid)->state, PROC_STATE_READY);
    return 0;
}

TEST(scheduler_empty_no_processes) {
    process_init();
    scheduler_init();
    ASSERT_EQ(scheduler_ready_count(), 0);
    /* Yield and block should be safe with no current process */
    current_pid = 0;
    scheduler_yield();
    scheduler_block();
    return 0;
}

TEST(scheduler_many_processes) {
    process_init();
    scheduler_init();
    /* Spawn many processes */
    int count = 0;
    for (int i = 0; i < 30; i++) {
        if (process_spawn("test.wex", PROC_PRIORITY_NORMAL) > 0) count++;
    }
    ASSERT_EQ(scheduler_ready_count(), count);
    return 0;
}

TEST(scheduler_unblock_invalid_pid) {
    process_init();
    scheduler_init();
    /* Should not crash */
    scheduler_unblock(9999);
    return 0;
}

static test_entry_t scheduler_extended_tests[] = {
    TEST_ENTRY(scheduler_init_resets_config, "Scheduler Extended"),
    TEST_ENTRY(scheduler_yield_from_ready_state, "Scheduler Extended"),
    TEST_ENTRY(scheduler_block_from_ready_state, "Scheduler Extended"),
    TEST_ENTRY(scheduler_unblock_non_blocked_process, "Scheduler Extended"),
    TEST_ENTRY(scheduler_round_trip_running_ready_running, "Scheduler Extended"),
    TEST_ENTRY(scheduler_round_trip_running_blocked_ready, "Scheduler Extended"),
    TEST_ENTRY(scheduler_multiple_yield_cycles, "Scheduler Extended"),
    TEST_ENTRY(scheduler_multiple_block_unblock_cycles, "Scheduler Extended"),
    TEST_ENTRY(scheduler_ready_count_with_blocked, "Scheduler Extended"),
    TEST_ENTRY(scheduler_ready_count_with_zombie, "Scheduler Extended"),
    TEST_ENTRY(scheduler_set_config_priority, "Scheduler Extended"),
    TEST_ENTRY(scheduler_get_config, "Scheduler Extended"),
    TEST_ENTRY(scheduler_get_config_null, "Scheduler Extended"),
    TEST_ENTRY(scheduler_config_change_during_operation, "Scheduler Extended"),
    TEST_ENTRY(scheduler_empty_no_processes, "Scheduler Extended"),
    TEST_ENTRY(scheduler_many_processes, "Scheduler Extended"),
    TEST_ENTRY(scheduler_unblock_invalid_pid, "Scheduler Extended"),
};

int main(void) {
    RUN_TESTS(scheduler_extended_tests);
}
