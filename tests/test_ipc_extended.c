/**
 * WebOS Extended IPC Tests
 *
 * Send/receive patterns, message ordering, boundary conditions, and edge cases.
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

/* Include process.h first so pid_t is defined for scheduler stubs */
#include "../kernel/src/process.h"

/* Stub out scheduler functions for IPC testing */
void scheduler_unblock(pid_t pid) { (void)pid; }
void scheduler_block(void) {}

#include "../kernel/src/process.c"
#include "../kernel/src/ipc.c"

/* ---- Tests ---- */

TEST(ipc_send_to_self) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("self.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    const char* data = "self_msg";
    int32_t result = ipc_send(pid, 1, data, 8);
    ASSERT_EQ(result, 0);
    /* Should be able to receive our own message */
    ipc_msg_t msg;
    result = ipc_recv_nonblock(&msg);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(msg.sender, pid);
    return 0;
}

TEST(ipc_multiple_senders_to_one_receiver) {
    process_init();
    ipc_init();
    pid_t sender1 = process_spawn("s1.wex", PROC_PRIORITY_NORMAL);
    pid_t sender2 = process_spawn("s2.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);

    /* Sender 1 sends */
    current_pid = sender1;
    ipc_send(receiver, IPC_MSG_DATA, "from1", 5);

    /* Sender 2 sends */
    current_pid = sender2;
    ipc_send(receiver, IPC_MSG_DATA, "from2", 5);

    /* Receiver gets both */
    current_pid = receiver;
    ipc_msg_t msg1, msg2;
    ASSERT_EQ(ipc_recv_nonblock(&msg1), 0);
    ASSERT_EQ(msg1.sender, sender1);
    ASSERT_EQ(ipc_recv_nonblock(&msg2), 0);
    ASSERT_EQ(msg2.sender, sender2);
    return 0;
}

TEST(ipc_call_reply_pattern) {
    process_init();
    ipc_init();
    pid_t caller = process_spawn("caller.wex", PROC_PRIORITY_NORMAL);
    pid_t callee = process_spawn("callee.wex", PROC_PRIORITY_NORMAL);

    /* Caller sends request */
    current_pid = caller;
    const char* req = "request";
    int32_t result = ipc_send(callee, IPC_MSG_REQUEST, req, 7);
    ASSERT_EQ(result, 0);

    /* Callee receives request */
    current_pid = callee;
    ipc_msg_t req_msg;
    ASSERT_EQ(ipc_recv_nonblock(&req_msg), 0);
    ASSERT_EQ(req_msg.type, IPC_MSG_REQUEST);
    ASSERT_EQ(req_msg.sender, caller);

    /* Callee sends reply */
    const char* resp = "response";
    result = ipc_reply(caller, resp, 8);
    ASSERT_EQ(result, 0);

    /* Caller receives reply */
    current_pid = caller;
    ipc_msg_t resp_msg;
    ASSERT_EQ(ipc_recv_nonblock(&resp_msg), 0);
    ASSERT_EQ(resp_msg.type, IPC_MSG_RESPONSE);
    ASSERT_EQ(resp_msg.sender, callee);
    return 0;
}

TEST(ipc_message_ordering_fifo) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* Send messages in order */
    const char* msgs[] = {"first", "second", "third"};
    for (int i = 0; i < 3; i++) {
        ipc_send(receiver, IPC_MSG_DATA, msgs[i], 5);
    }

    /* Receive should be FIFO */
    current_pid = receiver;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.data[0], 'f'); /* "first" */
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.data[0], 's'); /* "second" */
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.data[0], 't'); /* "third" */
    return 0;
}

TEST(ipc_zero_length_message) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    int32_t result = ipc_send(receiver, IPC_MSG_SIGNAL, NULL, 0);
    ASSERT_EQ(result, 0);

    current_pid = receiver;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.length, 0);
    ASSERT_EQ(msg.type, IPC_MSG_SIGNAL);
    return 0;
}

TEST(ipc_max_size_message) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* Send a message of exactly IPC_MAX_MSG_SIZE */
    uint8_t big_data[IPC_MAX_MSG_SIZE];
    memset(big_data, 0xAB, sizeof(big_data));
    int32_t result = ipc_send(receiver, IPC_MSG_DATA, big_data, IPC_MAX_MSG_SIZE);
    ASSERT_EQ(result, 0);

    current_pid = receiver;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.length, IPC_MAX_MSG_SIZE);
    ASSERT_EQ(msg.data[0], 0xAB);
    ASSERT_EQ(msg.data[IPC_MAX_MSG_SIZE - 1], 0xAB);
    return 0;
}

TEST(ipc_multiple_messages_in_queue) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* Send multiple messages */
    for (int i = 0; i < 10; i++) {
        uint8_t data[4] = {(uint8_t)i, 0, 0, 0};
        ipc_send(receiver, IPC_MSG_DATA, data, 4);
    }

    /* Verify queue has all 10 */
    current_pid = receiver;
    for (int i = 0; i < 10; i++) {
        ipc_msg_t msg;
        ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
        ASSERT_EQ(msg.data[0], (uint8_t)i);
    }
    return 0;
}

TEST(ipc_send_to_nonexistent_process) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* Channel will be auto-allocated for target 9999 */
    /* The IPC system auto-allocates channels, so send to any PID succeeds */
    /* unless channel allocation fails. This is by design. */
    int32_t result = ipc_send(9999, IPC_MSG_DATA, "x", 1);
    /* Should succeed (channel auto-allocated), but the process doesn't exist */
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(ipc_receive_on_empty_channel) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;

    ipc_msg_t msg;
    int32_t result = ipc_recv_nonblock(&msg);
    ASSERT_EQ(result, -4); /* ERR_BUSY / no messages */
    return 0;
}

TEST(ipc_channel_auto_allocation) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;

    /* Sending to self should auto-allocate a channel */
    int32_t result = ipc_send(pid, 1, "x", 1);
    ASSERT_EQ(result, 0);

    /* Receiving should work on that auto-allocated channel */
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    return 0;
}

TEST(ipc_type_field_preservation) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* Send with different IPC message types */
    uint32_t types[] = {IPC_MSG_SYSCALL, IPC_MSG_SIGNAL, IPC_MSG_DATA,
                        IPC_MSG_REQUEST, IPC_MSG_RESPONSE, IPC_MSG_EVENT};
    for (int i = 0; i < 6; i++) {
        ipc_send(receiver, types[i], "x", 1);
    }

    current_pid = receiver;
    for (int i = 0; i < 6; i++) {
        ipc_msg_t msg;
        ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
        ASSERT_EQ(msg.type, types[i]);
    }
    return 0;
}

TEST(ipc_data_integrity_long_message) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* Send a longer message with a specific pattern */
    uint8_t pattern[256];
    for (int i = 0; i < 256; i++) {
        pattern[i] = (uint8_t)(i ^ 0xAA);
    }
    ipc_send(receiver, IPC_MSG_DATA, pattern, 256);

    current_pid = receiver;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.length, 256);
    for (int i = 0; i < 256; i++) {
        ASSERT_EQ(msg.data[i], (uint8_t)(i ^ 0xAA));
    }
    return 0;
}

TEST(ipc_send_null_data_with_nonzero_length) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = sender;

    /* NULL data with non-zero length - should not crash, data should be zeroed */
    int32_t result = ipc_send(receiver, IPC_MSG_DATA, NULL, 4);
    ASSERT_EQ(result, 0);

    current_pid = receiver;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.length, 4);
    return 0;
}

TEST(ipc_recv_nonblock_null_msg) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    /* Should not crash with NULL msg pointer */
    int32_t result = ipc_recv_nonblock(NULL);
    /* With no messages, should return -4 */
    ASSERT_EQ(result, -4);
    return 0;
}

TEST(ipc_reply_uses_response_type) {
    process_init();
    ipc_init();
    pid_t sender = process_spawn("s.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("r.wex", PROC_PRIORITY_NORMAL);
    current_pid = receiver;

    /* Reply is just a send with IPC_MSG_RESPONSE type */
    int32_t result = ipc_reply(sender, "ok", 2);
    ASSERT_EQ(result, 0);

    current_pid = sender;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.type, IPC_MSG_RESPONSE);
    return 0;
}

TEST(ipc_channel_capacity_matches_constant) {
    process_init();
    ipc_init();
    pid_t pid = process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;
    /* Trigger channel allocation */
    ipc_send(pid, 1, "x", 1);

    /* Find the channel and verify capacity */
    int found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (channels[i].owner == pid) {
            ASSERT_EQ(channels[i].capacity, IPC_MAX_PENDING);
            found = 1;
            break;
        }
    }
    ASSERT_TRUE(found);
    return 0;
}

static test_entry_t ipc_extended_tests[] = {
    TEST_ENTRY(ipc_send_to_self, "IPC Extended"),
    TEST_ENTRY(ipc_multiple_senders_to_one_receiver, "IPC Extended"),
    TEST_ENTRY(ipc_call_reply_pattern, "IPC Extended"),
    TEST_ENTRY(ipc_message_ordering_fifo, "IPC Extended"),
    TEST_ENTRY(ipc_zero_length_message, "IPC Extended"),
    TEST_ENTRY(ipc_max_size_message, "IPC Extended"),
    TEST_ENTRY(ipc_multiple_messages_in_queue, "IPC Extended"),
    TEST_ENTRY(ipc_send_to_nonexistent_process, "IPC Extended"),
    TEST_ENTRY(ipc_receive_on_empty_channel, "IPC Extended"),
    TEST_ENTRY(ipc_channel_auto_allocation, "IPC Extended"),
    TEST_ENTRY(ipc_type_field_preservation, "IPC Extended"),
    TEST_ENTRY(ipc_data_integrity_long_message, "IPC Extended"),
    TEST_ENTRY(ipc_send_null_data_with_nonzero_length, "IPC Extended"),
    TEST_ENTRY(ipc_recv_nonblock_null_msg, "IPC Extended"),
    TEST_ENTRY(ipc_reply_uses_response_type, "IPC Extended"),
    TEST_ENTRY(ipc_channel_capacity_matches_constant, "IPC Extended"),
};

int main(void) {
    RUN_TESTS(ipc_extended_tests);
}
