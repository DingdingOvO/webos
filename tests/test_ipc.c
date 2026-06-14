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

TEST(ipc_init_clears_channels) {
    ipc_init();
    for (int i = 0; i < MAX_PROCESSES; i++) {
        ASSERT_EQ(channels[i].owner, 0);
        ASSERT_EQ(channels[i].msg_count, 0);
    }
    return 0;
}

TEST(ipc_send_basic) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    /* Set current_pid for getpid */
    current_pid = pid;
    const char* data = "hello";
    int32_t result = ipc_send(pid, 1, data, 5);
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(ipc_send_too_large) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    char big_data[IPC_MAX_MSG_SIZE + 100];
    memset(big_data, 0, sizeof(big_data));
    int32_t result = ipc_send(pid, 1, big_data, IPC_MAX_MSG_SIZE + 100);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(ipc_recv_nonblock_no_messages) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    ipc_msg_t msg;
    int32_t result = ipc_recv_nonblock(&msg);
    ASSERT_EQ(result, -4); /* ERR_BUSY / no messages */
    return 0;
}

TEST(ipc_send_and_recv) {
    process_init();
    ipc_init();
    pid_t pid1 = process_spawn("sender.wex", PROC_PRIORITY_NORMAL);
    pid_t pid2 = process_spawn("receiver.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid1;
    
    const char* data = "test_msg";
    int32_t result = ipc_send(pid2, 42, data, 8);
    ASSERT_EQ(result, 0);
    
    current_pid = pid2;
    ipc_msg_t msg;
    result = ipc_recv_nonblock(&msg);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(msg.sender, pid1);
    ASSERT_EQ(msg.type, 42);
    ASSERT_EQ(msg.length, 8);
    return 0;
}

TEST(ipc_recv_preserves_data) {
    process_init();
    ipc_init();
    pid_t pid1 = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t pid2 = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid1;
    
    const char* data = "ABCD";
    ipc_send(pid2, 1, data, 4);
    
    current_pid = pid2;
    ipc_msg_t msg;
    ipc_recv_nonblock(&msg);
    ASSERT_EQ(msg.data[0], 'A');
    ASSERT_EQ(msg.data[1], 'B');
    ASSERT_EQ(msg.data[2], 'C');
    ASSERT_EQ(msg.data[3], 'D');
    return 0;
}

TEST(ipc_channel_overflow) {
    process_init();
    ipc_init();
    pid_t pid1 = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t pid2 = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid1;
    
    /* Fill the channel */
    const char* data = "x";
    int last_result = 0;
    for (int i = 0; i < IPC_MAX_PENDING + 5; i++) {
        last_result = ipc_send(pid2, 1, data, 1);
    }
    /* At some point it should return ERR_BUSY */
    ASSERT_EQ(last_result, -4);
    return 0;
}

static test_entry_t ipc_tests[] = {
    TEST_ENTRY(ipc_init_clears_channels, "IPC"),
    TEST_ENTRY(ipc_send_basic, "IPC"),
    TEST_ENTRY(ipc_send_too_large, "IPC"),
    TEST_ENTRY(ipc_recv_nonblock_no_messages, "IPC"),
    TEST_ENTRY(ipc_send_and_recv, "IPC"),
    TEST_ENTRY(ipc_recv_preserves_data, "IPC"),
    TEST_ENTRY(ipc_channel_overflow, "IPC"),
};

int main(void) {
    RUN_TESTS(ipc_tests);
}
