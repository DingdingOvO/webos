/**
 * WebOS Kernel - Main Entry Point
 * 
 * The kernel is the core of WebOS, providing process management,
 * memory management, IPC, scheduling, and system call dispatch.
 */

#include <stdint.h>
#include "memory.h"
#include "process.h"
#include "scheduler.h"
#include "ipc.h"
#include "syscall.h"
#include "dynlink.h"
#include "host_func.h"

/* Kernel state */
static int kernel_initialized = 0;

/**
 * Kernel entry point - called by the bootloader.
 */
int main(void) {
    kernel_log("WebOS Kernel v0.1");
    kernel_log("Initializing subsystems...");

    /* Initialize memory management */
    if (memory_init() != 0) {
        kernel_log("FATAL: Memory initialization failed");
        return -1;
    }
    kernel_log("  Memory: OK");

    /* Initialize process management */
    if (process_init() != 0) {
        kernel_log("FATAL: Process initialization failed");
        return -2;
    }
    kernel_log("  Processes: OK");

    /* Initialize scheduler */
    if (scheduler_init() != 0) {
        kernel_log("FATAL: Scheduler initialization failed");
        return -3;
    }
    kernel_log("  Scheduler: OK");

    /* Initialize IPC */
    if (ipc_init() != 0) {
        kernel_log("FATAL: IPC initialization failed");
        return -4;
    }
    kernel_log("  IPC: OK");

    /* Initialize dynamic linker */
    if (dynlink_init() != 0) {
        kernel_log("FATAL: Dynamic linker initialization failed");
        return -5;
    }
    kernel_log("  DynLink: OK");

    /* Initialize syscall dispatch */
    syscall_init();
    kernel_log("  Syscalls: OK");

    kernel_initialized = 1;
    kernel_log("Kernel initialized successfully");

    /* Start the scheduler - this begins process execution */
    kernel_log("Starting scheduler...");
    scheduler_run();

    return 0;
}

/**
 * Check if kernel is initialized.
 */
int kernel_is_initialized(void) {
    return kernel_initialized;
}

/**
 * Kernel log function - outputs via host bridge.
 */
void kernel_log(const char* msg) {
    int len = 0;
    while (msg[len] != '\0') len++;
    host_syscall_invoke(1, (int32_t)msg, len, 0, 0, 0);
}
