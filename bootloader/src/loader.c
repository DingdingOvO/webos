/**
 * WebOS Module Loader Implementation
 * 
 * Uses the dynamic linker syscalls to load and manage modules.
 */

#include "loader.h"

/* Import syscall from host bridge */
__attribute__((import_module("webos"), import_name("syscall_invoke")))
extern int32_t syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                               int32_t a3, int32_t a4, int32_t a5);

/* Syscall numbers */
#define SYS_DYNLINK_LOAD   40
#define SYS_DYNLINK_UNLOAD 41
#define SYS_DYNLINK_SYM    42

module_handle_t load_module(const char* path) {
    /* Find path length */
    int len = 0;
    while (path[len] != '\0') len++;

    int32_t result = syscall_invoke(SYS_DYNLINK_LOAD,
                                     (int32_t)path, len, 0, 0, 0);
    if (result <= 0) {
        return INVALID_MODULE;
    }
    return (module_handle_t)result;
}

void* module_get_func(module_handle_t handle, const char* name) {
    int len = 0;
    while (name[len] != '\0') len++;

    void* func_ptr = (void*)0;
    int32_t result = syscall_invoke(SYS_DYNLINK_SYM,
                                     (int32_t)handle,
                                     (int32_t)name, len,
                                     (int32_t)&func_ptr, 0);
    if (result != 0) {
        return (void*)0;
    }
    return func_ptr;
}

int32_t module_unload(module_handle_t handle) {
    return syscall_invoke(SYS_DYNLINK_UNLOAD, (int32_t)handle, 0, 0, 0, 0);
}
