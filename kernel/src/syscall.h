#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/**
 * WebOS Kernel - Syscall Interface (Kernel Side)
 * 
 * This is the kernel-side syscall interface. It handles
 * syscall dispatch from user-space modules.
 * 
 * Note: The user-space syscall interface is in libs/syscall.h
 */

/* Forward declaration */
struct process;

/* Syscall handler function type */
typedef int32_t (*syscall_handler_t)(int32_t a1, int32_t a2,
                                      int32_t a3, int32_t a4,
                                      int32_t a5);

/**
 * Initialize the syscall dispatch table.
 */
void syscall_init(void);

/**
 * Dispatch a syscall.
 * 
 * @param num  Syscall number
 * @param a1-a5  Syscall arguments
 * @return  Return value
 */
int32_t syscall_dispatch(uint32_t num,
                          int32_t a1, int32_t a2,
                          int32_t a3, int32_t a4,
                          int32_t a5);

/**
 * Register a syscall handler.
 * 
 * @param num      Syscall number
 * @param handler  Handler function
 * @return         0 on success, negative on error
 */
int32_t syscall_register(uint32_t num, syscall_handler_t handler);

#endif /* SYSCALL_H */
