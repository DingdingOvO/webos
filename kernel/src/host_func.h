#ifndef HOST_FUNC_H
#define HOST_FUNC_H

/**
 * WebOS Kernel - Host Function Bridge
 * 
 * Defines the interface for calling host-provided functions
 * from the kernel. These are WASM imported functions.
 */

#include <stdint.h>

/**
 * Invoke a host syscall.
 * This is the primary bridge between kernel and host runtime.
 * 
 * @param num   Syscall number
 * @param a1-a5 Syscall arguments
 * @return      Return value from host
 */
__attribute__((import_module("webos"), import_name("syscall_invoke")))
extern int32_t host_syscall_invoke(uint32_t num,
                                    int32_t a1, int32_t a2,
                                    int32_t a3, int32_t a4,
                                    int32_t a5);

/**
 * Kernel log function.
 * Outputs a message via the host bridge.
 */
void kernel_log(const char* msg);

#endif /* HOST_FUNC_H */
