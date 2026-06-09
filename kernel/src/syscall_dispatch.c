/**
 * WebOS Kernel - Syscall Dispatch
 * 
 * This file contains the top-level syscall dispatch entry point
 * and connects to the host bridge.
 */

#include "syscall.h"
#include "host_func.h"

/**
 * Main syscall entry point called from user-space.
 * Validates the syscall number and dispatches to the handler.
 */
int32_t syscall_dispatch(uint32_t num,
                          int32_t a1, int32_t a2,
                          int32_t a3, int32_t a4,
                          int32_t a5);

/**
 * Handle an unknown/unimplemented syscall.
 */
static int32_t syscall_unimplemented(uint32_t num) {
    kernel_log("Unimplemented syscall");
    return -8;  /* ERR_NOSYS */
}
