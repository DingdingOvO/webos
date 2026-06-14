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

TEST(process_init_clears_table) {
    process_init();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        ASSERT_EQ(process_table[i].pid, 0);
    }
    return 0;
}

TEST(process_spawn_returns_pid) {
    process_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    ASSERT_TRUE(pid > 0);
    return 0;
}

TEST(process_spawn_increments_pid) {
    process_init();
    pid_t pid1 = process_spawn("a.wex", PROC_PRIORITY_NORMAL);
    pid_t pid2 = process_spawn("b.wex", PROC_PRIORITY_NORMAL);
    ASSERT_TRUE(pid2 > pid1);
    return 0;
}

TEST(process_spawn_sets_ready_state) {
    process_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    process_t* proc = process_get(pid);
    ASSERT_NOT_NULL(proc);
    ASSERT_EQ(proc->state, PROC_STATE_READY);
    return 0;
}

TEST(process_spawn_sets_priority) {
    process_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_HIGH);
    process_t* proc = process_get(pid);
    ASSERT_EQ(proc->priority, PROC_PRIORITY_HIGH);
    return 0;
}

TEST(process_get_returns_null_for_invalid) {
    process_init();
    process_t* proc = process_get(999);
    ASSERT_NULL(proc);
    return 0;
}

TEST(process_kill_marks_zombie) {
    process_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    int32_t result = process_kill(pid);
    ASSERT_EQ(result, 0);
    process_t* proc = process_get(pid);
    ASSERT_EQ(proc->state, PROC_STATE_ZOMBIE);
    return 0;
}

TEST(process_kill_invalid_returns_error) {
    process_init();
    int32_t result = process_kill(999);
    ASSERT_EQ(result, -2);
    return 0;
}

TEST(process_getpid_initial) {
    process_init();
    ASSERT_EQ(process_getpid(), 0);
    return 0;
}

TEST(process_wait_zombie_reaps) {
    process_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    process_kill(pid);
    int32_t exit_code = 0;
    int32_t result = process_wait(pid, &exit_code);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(exit_code, -1); /* killed, not exited */
    return 0;
}

TEST(process_wait_running_returns_busy) {
    process_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    int32_t exit_code = 0;
    int32_t result = process_wait(pid, &exit_code);
    ASSERT_EQ(result, -4); /* ERR_BUSY */
    return 0;
}

TEST(process_spawn_max_processes) {
    process_init();
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES + 10; i++) {
        pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
        if (pid > 0) count++;
    }
    ASSERT_EQ(count, MAX_PROCESSES);
    return 0;
}

static test_entry_t process_tests[] = {
    TEST_ENTRY(process_init_clears_table, "Process"),
    TEST_ENTRY(process_spawn_returns_pid, "Process"),
    TEST_ENTRY(process_spawn_increments_pid, "Process"),
    TEST_ENTRY(process_spawn_sets_ready_state, "Process"),
    TEST_ENTRY(process_spawn_sets_priority, "Process"),
    TEST_ENTRY(process_get_returns_null_for_invalid, "Process"),
    TEST_ENTRY(process_kill_marks_zombie, "Process"),
    TEST_ENTRY(process_kill_invalid_returns_error, "Process"),
    TEST_ENTRY(process_getpid_initial, "Process"),
    TEST_ENTRY(process_wait_zombie_reaps, "Process"),
    TEST_ENTRY(process_wait_running_returns_busy, "Process"),
    TEST_ENTRY(process_spawn_max_processes, "Process"),
};

int main(void) {
    RUN_TESTS(process_tests);
}
