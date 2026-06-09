#ifndef DYNLINK_H
#define DYNLINK_H

#include <stdint.h>

/* Module handle type */
typedef uint32_t module_handle_t;
#define INVALID_MODULE_HANDLE 0

/* Maximum loaded modules */
#define MAX_LOADED_MODULES 32

/* Module info tracked by the dynamic linker */
typedef struct {
    module_handle_t handle;
    char name[64];
    void* wasm_module;      /* WebAssembly.Module handle */
    void* wasm_instance;    /* WebAssembly.Instance handle */
    uint32_t ref_count;     /* Reference count */
    uint32_t type;          /* Module type (wex, Wdll, wli) */
} dynlink_module_t;

/**
 * Initialize the dynamic linker.
 */
int32_t dynlink_init(void);

/**
 * Load a dynamic module by name.
 * 
 * @param name      Module name (e.g., "gpu_driver.Wdll")
 * @param name_len  Length of name string
 * @return          Module handle, or INVALID_MODULE_HANDLE on error
 */
module_handle_t dynlink_load(const char* name, int32_t name_len);

/**
 * Unload a dynamic module.
 * 
 * @param handle  Module handle
 * @return        0 on success, negative on error
 */
int32_t dynlink_unload(module_handle_t handle);

/**
 * Resolve a symbol from a loaded module.
 * 
 * @param handle   Module handle
 * @param sym      Symbol name
 * @param sym_len  Symbol name length
 * @param out      Output: function pointer
 * @return         0 on success, negative on error
 */
int32_t dynlink_resolve(module_handle_t handle,
                         const char* sym, int32_t sym_len,
                         void** out);

/**
 * Get module info by handle.
 */
dynlink_module_t* dynlink_get_info(module_handle_t handle);

/**
 * Get module info by name.
 */
dynlink_module_t* dynlink_get_by_name(const char* name);

#endif /* DYNLINK_H */
