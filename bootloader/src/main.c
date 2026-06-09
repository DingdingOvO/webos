/**
 * WebOS Bootloader - Main Entry Point
 * 
 * The bootloader is the first WASM module loaded by the host.
 * It performs hardware initialization and loads the kernel.
 */

#include <stdint.h>
#include "loader.h"
#include "gpu_init.h"

/* Boot stages */
#define BOOT_STAGE_INIT     0
#define BOOT_STAGE_GPU      1
#define BOOT_STAGE_KERNEL   2
#define BOOT_STAGE_COMPLETE 3

static int boot_stage = BOOT_STAGE_INIT;

/**
 * Boot entry point - called by the host runtime.
 * 
 * Initializes hardware abstraction, sets up the GPU,
 * and loads the kernel module.
 */
int main(void) {
    /* Stage 0: Basic initialization */
    boot_stage = BOOT_STAGE_INIT;
    boot_log("WebOS Bootloader v0.1");
    boot_log("Initializing...");

    /* Stage 1: Initialize GPU */
    boot_stage = BOOT_STAGE_GPU;
    boot_log("Initializing GPU...");
    if (gpu_init() != 0) {
        boot_log("ERROR: GPU initialization failed");
        return -1;
    }
    boot_log("GPU initialized");

    /* Stage 2: Load kernel */
    boot_stage = BOOT_STAGE_KERNEL;
    boot_log("Loading kernel...");
    
    module_handle_t kernel = load_module("kernel.wasm");
    if (kernel == INVALID_MODULE) {
        boot_log("ERROR: Failed to load kernel");
        return -2;
    }
    boot_log("Kernel loaded");

    /* Stage 3: Boot complete */
    boot_stage = BOOT_STAGE_COMPLETE;
    boot_log("Boot complete, transferring control to kernel");

    return 0;
}

/**
 * Get current boot stage (for status reporting).
 */
int get_boot_stage(void) {
    return boot_stage;
}

/**
 * Print a boot log message via the host bridge.
 * This uses the SYS_PRINT syscall.
 */
void boot_log(const char* msg) {
    /* Find string length */
    int len = 0;
    while (msg[len] != '\0') len++;
    
    /* Use syscall to print */
    sys_print(msg, len);
}

/* Import syscall from host bridge */
__attribute__((import_module("webos"), import_name("syscall_invoke")))
extern int32_t syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                               int32_t a3, int32_t a4, int32_t a5);

static inline int32_t sys_print(const char* msg, int32_t len) {
    return syscall_invoke(1, (int32_t)msg, len, 0, 0, 0);
}
