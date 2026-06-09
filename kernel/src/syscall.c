/**
 * WebOS Kernel - Syscall Dispatch
 * 
 * Dispatches system calls from user-space to kernel handlers.
 */

#include "syscall.h"
#include "host_func.h"

/* Maximum syscall number */
#define MAX_SYSCALL 128

/* Syscall dispatch table */
static syscall_handler_t syscall_table[MAX_SYSCALL];

void syscall_init(void) {
    /* Clear the syscall table */
    for (int i = 0; i < MAX_SYSCALL; i++) {
        syscall_table[i] = (syscall_handler_t)0;
    }

    /* Register core syscalls */
    extern int32_t sys_print(int32_t, int32_t, int32_t, int32_t, int32_t);
    extern int32_t sys_getpid(int32_t, int32_t, int32_t, int32_t, int32_t);
    extern int32_t sys_exit(int32_t, int32_t, int32_t, int32_t, int32_t);
    extern int32_t sys_spawn(int32_t, int32_t, int32_t, int32_t, int32_t);
    extern int32_t sys_ipc_send(int32_t, int32_t, int32_t, int32_t, int32_t);
    extern int32_t sys_ipc_recv(int32_t, int32_t, int32_t, int32_t, int32_t);

    syscall_register(1, sys_print);     /* SYS_PRINT */
    syscall_register(22, sys_getpid);   /* SYS_GETPID */
    syscall_register(24, sys_exit);     /* SYS_EXIT */
    syscall_register(20, sys_spawn);    /* SYS_SPAWN */
    syscall_register(30, sys_ipc_send); /* SYS_IPC_SEND */
    syscall_register(31, sys_ipc_recv); /* SYS_IPC_RECV */
}

int32_t syscall_dispatch(uint32_t num,
                          int32_t a1, int32_t a2,
                          int32_t a3, int32_t a4,
                          int32_t a5) {
    if (num >= MAX_SYSCALL || !syscall_table[num]) {
        return -8;  /* ERR_NOSYS */
    }
    return syscall_table[num](a1, a2, a3, a4, a5);
}

int32_t syscall_register(uint32_t num, syscall_handler_t handler) {
    if (num >= MAX_SYSCALL) return -1;  /* ERR_INVAL */
    syscall_table[num] = handler;
    return 0;
}

/* Core syscall implementations */

#include "process.h"
#include "ipc.h"

int32_t sys_print(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    /* Forward to host for console output */
    return host_syscall_invoke(1, a1, a2, a3, a4, a5);
}

int32_t sys_getpid(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    return (int32_t)process_getpid();
}

int32_t sys_exit(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    process_exit(a1);
    return 0;  /* Never reached */
}

int32_t sys_spawn(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    const char* path = (const char*)(uintptr_t)a1;
    pid_t pid = process_spawn(path, PROC_PRIORITY_NORMAL);
    return (int32_t)pid;
}

int32_t sys_ipc_send(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    pid_t target = (pid_t)a1;
    uint32_t type = (uint32_t)a2;
    const void* data = (const void*)(uintptr_t)a3;
    uint32_t len = (uint32_t)a4;
    return ipc_send(target, type, data, len);
}

int32_t sys_ipc_recv(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5) {
    ipc_msg_t* msg = (ipc_msg_t*)(uintptr_t)a1;
    return ipc_recv_nonblock(msg);
}
