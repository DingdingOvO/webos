/**
 * WebOS Kernel - Dynamic Linker
 * 
 * Loads and manages shared WASM modules (.Wdll).
 * Handles symbol resolution and module dependencies.
 */

#include "dynlink.h"
#include "host_func.h"

/* Module table */
static dynlink_module_t module_table[MAX_LOADED_MODULES];
static module_handle_t next_handle = 1;

int32_t dynlink_init(void) {
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        module_table[i].handle = INVALID_MODULE_HANDLE;
        module_table[i].name[0] = '\0';
        module_table[i].wasm_module = (void*)0;
        module_table[i].wasm_instance = (void*)0;
        module_table[i].ref_count = 0;
        module_table[i].type = 0;
    }
    next_handle = 1;
    return 0;
}

module_handle_t dynlink_load(const char* name, int32_t name_len) {
    /* Check if already loaded */
    dynlink_module_t* existing = dynlink_get_by_name(name);
    if (existing) {
        existing->ref_count++;
        return existing->handle;
    }

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (module_table[i].handle == INVALID_MODULE_HANDLE) {
            slot = i;
            break;
        }
    }

    if (slot < 0) return INVALID_MODULE_HANDLE;  /* No free slots */

    /* Copy name */
    int copy_len = name_len < 63 ? name_len : 63;
    for (int i = 0; i < copy_len; i++) {
        module_table[slot].name[i] = name[i];
    }
    module_table[slot].name[copy_len] = '\0';

    /* In the full implementation, we would:
     * 1. Request the host to fetch the WASM file
     * 2. Compile it to WebAssembly.Module
     * 3. Read the module_type custom section
     * 4. Instantiate with appropriate imports
     * For now, we delegate to the host via syscall.
     */

    int32_t result = host_syscall_invoke(40, (int32_t)name, name_len, 0, 0, 0);
    if (result <= 0) return INVALID_MODULE_HANDLE;

    module_table[slot].handle = (module_handle_t)result;
    module_table[slot].wasm_module = (void*)(uintptr_t)result;
    module_table[slot].wasm_instance = (void*)0;
    module_table[slot].ref_count = 1;
    module_table[slot].type = 0;

    return module_table[slot].handle;
}

int32_t dynlink_unload(module_handle_t handle) {
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (module_table[i].handle == handle) {
            module_table[i].ref_count--;
            if (module_table[i].ref_count == 0) {
                /* Actually unload */
                host_syscall_invoke(41, (int32_t)handle, 0, 0, 0, 0);
                module_table[i].handle = INVALID_MODULE_HANDLE;
                module_table[i].name[0] = '\0';
                module_table[i].wasm_module = (void*)0;
                module_table[i].wasm_instance = (void*)0;
            }
            return 0;
        }
    }
    return -2;  /* ERR_NOENT */
}

int32_t dynlink_resolve(module_handle_t handle,
                         const char* sym, int32_t sym_len,
                         void** out) {
    if (!out) return -1;  /* ERR_INVAL */

    /* Delegate to host for symbol resolution */
    int32_t result = host_syscall_invoke(42, (int32_t)handle,
                                          (int32_t)sym, sym_len,
                                          (int32_t)out, 0);
    return result;
}

dynlink_module_t* dynlink_get_info(module_handle_t handle) {
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (module_table[i].handle == handle) {
            return &module_table[i];
        }
    }
    return (dynlink_module_t*)0;
}

dynlink_module_t* dynlink_get_by_name(const char* name) {
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (module_table[i].handle != INVALID_MODULE_HANDLE) {
            /* Compare names */
            int j = 0;
            while (name[j] && module_table[i].name[j] &&
                   name[j] == module_table[i].name[j]) {
                j++;
            }
            if (name[j] == '\0' && module_table[i].name[j] == '\0') {
                return &module_table[i];
            }
        }
    }
    return (dynlink_module_t*)0;
}
