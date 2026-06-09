#ifndef MODULE_TYPES_H
#define MODULE_TYPES_H

/**
 * WebOS Module Type Definitions
 * 
 * These constants define the module types used in WebAssembly custom sections.
 * The dynamic loader reads the "module_type" custom section to determine
 * how to load and instantiate each module.
 * 
 * File extensions (.wex, .Wdll, .wli) are for human identification only.
 * The actual behavior is determined by the binary content.
 */

/* Module type string constants (stored in WASM custom section "module_type") */
#define MODULE_TYPE_WEX   "wex"     /* Executable - standalone program */
#define MODULE_TYPE_WDLL  "Wdll"    /* Dynamic Link Library - shared library */
#define MODULE_TYPE_WLI   "wli"     /* Library Interface - symbol description */

/* Maximum module type string length */
#define MODULE_TYPE_MAX_LEN 8

/* Module type enum for C code */
typedef enum {
    MODULE_KIND_UNKNOWN = 0,
    MODULE_KIND_WEX = 1,       /* Executable */
    MODULE_KIND_WDLL = 2,      /* Dynamic library */
    MODULE_KIND_WLI = 3,       /* Interface description */
} module_kind_t;

/* Module info structure returned by the loader */
typedef struct {
    module_kind_t kind;
    char type_str[MODULE_TYPE_MAX_LEN];
    void* wasm_module;      /* WebAssembly.Module handle (opaque) */
    void* wasm_instance;    /* WebAssembly.Instance handle (opaque) */
} module_info_t;

#endif /* MODULE_TYPES_H */
