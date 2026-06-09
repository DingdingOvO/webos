#ifndef LOADER_H
#define LOADER_H

/**
 * WebOS Module Loader Interface
 * 
 * Provides functions for loading WASM modules and
 * obtaining handles to them.
 */

#include <stdint.h>

/* Module handle type */
typedef uint32_t module_handle_t;
#define INVALID_MODULE 0

/**
 * Load a WASM module by path.
 * 
 * @param path  Module path relative to /wasm/
 * @return      Module handle, or INVALID_MODULE on failure
 */
module_handle_t load_module(const char* path);

/**
 * Get a function pointer from a loaded module.
 * 
 * @param handle  Module handle
 * @param name    Export function name
 * @return        Function pointer, or NULL on failure
 */
void* module_get_func(module_handle_t handle, const char* name);

/**
 * Unload a previously loaded module.
 * 
 * @param handle  Module handle
 * @return        0 on success, negative on error
 */
int32_t module_unload(module_handle_t handle);

/**
 * Print a boot log message.
 * 
 * @param msg  Message string (null-terminated)
 */
void boot_log(const char* msg);

/**
 * Get current boot stage.
 * 
 * @return  Boot stage number (0-3)
 */
int get_boot_stage(void);

#endif /* LOADER_H */
