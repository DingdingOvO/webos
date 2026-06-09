/**
 * WebOS Kernel Integration Tests
 *
 * Tests that exercise multiple kernel subsystems together:
 * - Full boot sequence
 * - Process + IPC interaction
 * - Memory allocation from syscall context
 * - Scheduler + process lifecycle
 * - Dynamic linker + process spawn
 */

#include "test_framework.h"

/* Skip WASM-specific host_func.h and provide stubs */
#define HOST_FUNC_H
#include <stdint.h>
#include <string.h>

static int32_t host_syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                                    int32_t a3, int32_t a4, int32_t a5) {
    (void)num; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return 0;
}

static void kernel_log(const char* msg) { (void)msg; }

#include "../kernel/src/process.h"
#include "../kernel/src/scheduler.h"

#include "../kernel/src/memory.c"
#include "../kernel/src/process.c"
#include "../kernel/src/scheduler.c"
#include "../kernel/src/ipc.c"
#include "../kernel/src/syscall.c"
#include "../kernel/src/dynlink.c"

/* ---- Helper: simulate the kernel boot sequence ---- */
static void kernel_boot(void) {
    memory_init();
    process_init();
    scheduler_init();
    ipc_init();
    dynlink_init();
    syscall_init();
}

/* ---- Tests ---- */

TEST(full_boot_sequence) {
    /* Simulate the kernel initialization order from main.c */
    ASSERT_EQ(memory_init(), 0);
    ASSERT_EQ(process_init(), 0);
    ASSERT_EQ(scheduler_init(), 0);
    ASSERT_EQ(ipc_init(), 0);
    ASSERT_EQ(dynlink_init(), 0);
    /* syscall_init returns void */
    syscall_init();
    return 0;
}

TEST(boot_then_spawn_process) {
    kernel_boot();
    pid_t pid = process_spawn("init.wex", PROC_PRIORITY_NORMAL);
    ASSERT_TRUE(pid > 0);
    process_t* proc = process_get(pid);
    ASSERT_NOT_NULL(proc);
    ASSERT_EQ(proc->state, PROC_STATE_READY);
    return 0;
}

TEST(spawn_then_ipc_send_recv) {
    kernel_boot();
    pid_t sender = process_spawn("sender.wex", PROC_PRIORITY_NORMAL);
    pid_t receiver = process_spawn("receiver.wex", PROC_PRIORITY_NORMAL);

    /* Sender sends message */
    current_pid = sender;
    const char* data = "integration_test";
    int32_t result = ipc_send(receiver, IPC_MSG_DATA, data, 16);
    ASSERT_EQ(result, 0);

    /* Receiver gets message */
    current_pid = receiver;
    ipc_msg_t msg;
    result = ipc_recv_nonblock(&msg);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(msg.sender, sender);
    ASSERT_EQ(msg.type, IPC_MSG_DATA);
    ASSERT_EQ(msg.length, 16);
    return 0;
}

TEST(spawn_then_memory_alloc) {
    kernel_boot();
    pid_t pid = process_spawn("mem_user.wex", PROC_PRIORITY_NORMAL);
    current_pid = pid;

    /* Allocate memory on behalf of the process */
    void* ptr = memory_alloc(4);
    ASSERT_NOT_NULL(ptr);
    ASSERT_EQ(stats.used_pages, 4 + 4); /* 4 kernel + 4 allocated */

    /* Process should be able to use syscall for memory */
    /* Since SYS_MALLOC (10) is not in our core syscalls, test via direct call */
    memory_free(ptr);
    ASSERT_EQ(stats.used_pages, 4);
    return 0;
}

TEST(scheduler_process_lifecycle) {
    kernel_boot();
    pid_t p1 = process_spawn("a.wex", PROC_PRIORITY_NORMAL);
    pid_t p2 = process_spawn("b.wex", PROC_PRIORITY_NORMAL);
    pid_t p3 = process_spawn("c.wex", PROC_PRIORITY_NORMAL);

    /* All should be READY */
    ASSERT_EQ(scheduler_ready_count(), 3);

    /* Simulate running p1 */
    current_pid = p1;
    process_get(p1)->state = PROC_STATE_RUNNING;
    scheduler_yield();
    ASSERT_EQ(process_get(p1)->state, PROC_STATE_READY);

    /* Block p2 */
    current_pid = p2;
    process_get(p2)->state = PROC_STATE_RUNNING;
    scheduler_block();
    ASSERT_EQ(process_get(p2)->state, PROC_STATE_BLOCKED);
    ASSERT_EQ(scheduler_ready_count(), 2);

    /* Unblock p2 */
    scheduler_unblock(p2);
    ASSERT_EQ(process_get(p2)->state, PROC_STATE_READY);
    ASSERT_EQ(scheduler_ready_count(), 3);
    return 0;
}

TEST(spawn_kill_wait_cycle) {
    kernel_boot();
    pid_t pid = process_spawn("ephemeral.wex", PROC_PRIORITY_NORMAL);
    ASSERT_TRUE(pid > 0);

    process_kill(pid);
    process_t* proc = process_get(pid);
    ASSERT_EQ(proc->state, PROC_STATE_ZOMBIE);

    int32_t exit_code;
    int32_t result = process_wait(pid, &exit_code);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(exit_code, -1);

    /* Slot should be freed now */
    ASSERT_NULL(process_get(pid));
    return 0;
}

TEST(ipc_between_three_processes) {
    kernel_boot();
    pid_t p1 = process_spawn("p1.wex", PROC_PRIORITY_NORMAL);
    pid_t p2 = process_spawn("p2.wex", PROC_PRIORITY_NORMAL);
    pid_t p3 = process_spawn("p3.wex", PROC_PRIORITY_NORMAL);

    /* p1 sends to p2 */
    current_pid = p1;
    ASSERT_EQ(ipc_send(p2, IPC_MSG_DATA, "from_p1", 7), 0);

    /* p3 sends to p2 */
    current_pid = p3;
    ASSERT_EQ(ipc_send(p2, IPC_MSG_REQUEST, "from_p3", 7), 0);

    /* p2 receives both */
    current_pid = p2;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.sender, p1);
    ASSERT_EQ(msg.type, IPC_MSG_DATA);
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.sender, p3);
    ASSERT_EQ(msg.type, IPC_MSG_REQUEST);
    return 0;
}

TEST(memory_alloc_across_processes) {
    kernel_boot();
    /* Simulate multiple processes each allocating memory */
    void* m1 = memory_alloc(2);
    void* m2 = memory_alloc(3);
    void* m3 = memory_alloc(1);

    ASSERT_NOT_NULL(m1);
    ASSERT_NOT_NULL(m2);
    ASSERT_NOT_NULL(m3);

    uint32_t total_before = stats.used_pages;
    memory_free(m2);
    ASSERT_EQ(stats.used_pages, total_before - 3);

    /* New allocation should work */
    void* m4 = memory_alloc(1);
    ASSERT_NOT_NULL(m4);
    (void)m1; (void)m3; (void)m4;
    return 0;
}

TEST(syscall_dispatch_across_subsystems) {
    kernel_boot();
    /* Spawn a process via syscall */
    current_pid = 0;
    int32_t pid_result = syscall_dispatch(20, (int32_t)(uintptr_t)"test.wex", 0, 0, 0, 0);
    ASSERT_TRUE(pid_result > 0);

    /* Get PID via syscall */
    current_pid = (pid_t)pid_result;
    int32_t getpid_result = syscall_dispatch(22, 0, 0, 0, 0, 0);
    ASSERT_EQ(getpid_result, pid_result);
    return 0;
}

TEST(dynlink_init_then_process_spawn) {
    kernel_boot();
    /* Dynlink should be initialized, processes can be spawned */
    dynlink_module_t* mod = dynlink_get_by_name("nonexistent");
    ASSERT_NULL(mod);

    /* Spawning processes should still work */
    pid_t pid = process_spawn("app.wex", PROC_PRIORITY_NORMAL);
    ASSERT_TRUE(pid > 0);
    return 0;
}

TEST(shared_memory_ipc_pattern) {
    kernel_boot();
    pid_t p1 = process_spawn("shmem1.wex", PROC_PRIORITY_NORMAL);
    pid_t p2 = process_spawn("shmem2.wex", PROC_PRIORITY_NORMAL);

    /* Allocate shared memory */
    void* shared = memory_map_shared(PAGE_SIZE * 2, p1);
    ASSERT_NOT_NULL(shared);
    ASSERT_EQ(stats.shared_pages, 2);

    /* Notify via IPC that shared memory is ready */
    current_pid = p1;
    ipc_send(p2, IPC_MSG_SIGNAL, "ready", 5);

    /* p2 receives notification */
    current_pid = p2;
    ipc_msg_t msg;
    ASSERT_EQ(ipc_recv_nonblock(&msg), 0);
    ASSERT_EQ(msg.type, IPC_MSG_SIGNAL);
    return 0;
}

TEST(scheduler_config_affects_behavior) {
    kernel_boot();
    process_spawn("test.wex", PROC_PRIORITY_NORMAL);
    process_spawn("test2.wex", PROC_PRIORITY_NORMAL);

    /* Change scheduler config */
    sched_config_t cfg = { .algorithm = SCHED_PRIORITY, .time_slice_ms = 50 };
    ASSERT_EQ(scheduler_set_config(&cfg), 0);

    /* Verify config was set */
    sched_config_t out;
    scheduler_get_config(&out);
    ASSERT_EQ(out.algorithm, SCHED_PRIORITY);
    ASSERT_EQ(out.time_slice_ms, 50);

    /* Processes should still be schedulable */
    ASSERT_EQ(scheduler_ready_count(), 2);
    return 0;
}

TEST(full_process_lifecycle_with_ipc) {
    kernel_boot();
    pid_t server = process_spawn("server.wex", PROC_PRIORITY_NORMAL);
    pid_t client = process_spawn("client.wex", PROC_PRIORITY_NORMAL);

    /* Client sends request */
    current_pid = client;
    ipc_send(server, IPC_MSG_REQUEST, "req", 3);

    /* Server receives and replies */
    current_pid = server;
    ipc_msg_t req;
    ipc_recv_nonblock(&req);
    ASSERT_EQ(req.sender, client);
    ipc_reply(client, "resp", 4);

    /* Client gets response */
    current_pid = client;
    ipc_msg_t resp;
    ipc_recv_nonblock(&resp);
    ASSERT_EQ(resp.type, IPC_MSG_RESPONSE);
    ASSERT_EQ(resp.sender, server);

    /* Kill both and wait */
    process_kill(server);
    process_kill(client);
    int32_t ec;
    process_wait(server, &ec);
    process_wait(client, &ec);
    return 0;
}

TEST(memory_stats_after_full_boot) {
    kernel_boot();
    mem_stats_t s;
    memory_stats(&s);
    /* After boot, only kernel pages should be used */
    ASSERT_EQ(s.kernel_pages, 4);
    ASSERT_EQ(s.used_pages, 4);
    ASSERT_TRUE(s.total_pages > 0);
    ASSERT_EQ(s.total_pages, s.used_pages + s.free_pages);
    return 0;
}

static test_entry_t integration_kernel_tests[] = {
    TEST_ENTRY(full_boot_sequence, "Kernel Integration"),
    TEST_ENTRY(boot_then_spawn_process, "Kernel Integration"),
    TEST_ENTRY(spawn_then_ipc_send_recv, "Kernel Integration"),
    TEST_ENTRY(spawn_then_memory_alloc, "Kernel Integration"),
    TEST_ENTRY(scheduler_process_lifecycle, "Kernel Integration"),
    TEST_ENTRY(spawn_kill_wait_cycle, "Kernel Integration"),
    TEST_ENTRY(ipc_between_three_processes, "Kernel Integration"),
    TEST_ENTRY(memory_alloc_across_processes, "Kernel Integration"),
    TEST_ENTRY(syscall_dispatch_across_subsystems, "Kernel Integration"),
    TEST_ENTRY(dynlink_init_then_process_spawn, "Kernel Integration"),
    TEST_ENTRY(shared_memory_ipc_pattern, "Kernel Integration"),
    TEST_ENTRY(scheduler_config_affects_behavior, "Kernel Integration"),
    TEST_ENTRY(full_process_lifecycle_with_ipc, "Kernel Integration"),
    TEST_ENTRY(memory_stats_after_full_boot, "Kernel Integration"),
};

int main(void) {
    RUN_TESTS(integration_kernel_tests);
}
