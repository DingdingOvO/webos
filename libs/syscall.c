/**
 * WebOS Syscall Implementation (Client-side stub)
 * 
 * This file provides the client-side syscall invocation mechanism.
 * In the WASM environment, syscalls are implemented as imported
 * host functions that the runtime bridge provides.
 * 
 * When running natively (for testing), a simple stub is used.
 */

#include "syscall.h"

#ifdef __wasm32__

/* In WASM, syscall_invoke is an imported host function */
__attribute__((import_module("webos"), import_name("syscall_invoke")))
extern int32_t syscall_invoke(uint32_t syscall_num,
                               int32_t a1, int32_t a2,
                               int32_t a3, int32_t a4,
                               int32_t a5);

#else /* Native fallback for testing */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Native stub implementation */
int32_t syscall_invoke(uint32_t syscall_num,
                       int32_t a1, int32_t a2,
                       int32_t a3, int32_t a4,
                       int32_t a5) {
    switch (syscall_num) {
        case SYS_PRINT: {
            const char* msg = (const char*)a1;
            int32_t len = a2;
            printf("%.*s", len, msg);
            return ERR_OK;
        }
        case SYS_GETPID:
            return 1; /* Stub PID */
        case SYS_EXIT:
            exit(a1);
            return ERR_OK;
        case SYS_MALLOC: {
            void* ptr = malloc((size_t)a1);
            return (int32_t)ptr;
        }
        case SYS_FREE:
            free((void*)a1);
            return ERR_OK;
        default:
            fprintf(stderr, "Unimplemented syscall: %u\n", syscall_num);
            return ERR_NOSYS;
    }
}

#endif /* __wasm32__ */
