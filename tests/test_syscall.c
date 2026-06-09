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

/* Include process.h first so pid_t is defined for scheduler stubs */
#include "../kernel/src/process.h"

/* Stub out scheduler functions for IPC testing */
void scheduler_unblock(pid_t pid) { (void)pid; }
void scheduler_block(void) {}

#include "../kernel/src/process.c"
#include "../kernel/src/ipc.c"
#include "../kernel/src/syscall.c"

TEST(syscall_init_clears_table) {
    syscall_init();
    for (int i = 0; i < MAX_SYSCALL; i++) {
        ASSERT_TRUE(syscall_table[i] == NULL || i == 1 || i == 20 || i == 22 || i == 24 || i == 30 || i == 31);
    }
    return 0;
}

TEST(syscall_dispatch_invalid) {
    syscall_init();
    int32_t result = syscall_dispatch(200, 0, 0, 0, 0, 0);
    ASSERT_EQ(result, -8); /* ERR_NOSYS */
    return 0;
}

TEST(syscall_getpid) {
    process_init();
    syscall_init();
    current_pid = 42;
    int32_t result = syscall_dispatch(22, 0, 0, 0, 0, 0);
    ASSERT_EQ(result, 42);
    return 0;
}

TEST(syscall_register_returns_zero) {
    syscall_init();
    int32_t result = syscall_register(50, sys_getpid);
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(syscall_register_out_of_range) {
    int32_t result = syscall_register(MAX_SYSCALL, sys_getpid);
    ASSERT_EQ(result, -1);
    return 0;
}

static test_entry_t syscall_tests[] = {
    TEST_ENTRY(syscall_init_clears_table, "Syscall"),
    TEST_ENTRY(syscall_dispatch_invalid, "Syscall"),
    TEST_ENTRY(syscall_getpid, "Syscall"),
    TEST_ENTRY(syscall_register_returns_zero, "Syscall"),
    TEST_ENTRY(syscall_register_out_of_range, "Syscall"),
};

int main(void) {
    RUN_TESTS(syscall_tests);
}
