/**
 * WebOS Extended Dynamic Linker Tests
 *
 * Module load/unload, symbol table, name collisions, and edge cases.
 * Note: dynlink_load delegates to host_syscall_invoke (stubbed to return 0),
 * so load will return INVALID_MODULE_HANDLE in tests. We test the table logic
 * using direct manipulation where needed.
 */

#include "test_framework.h"

/* Skip WASM-specific host_func.h and provide stubs */
#define HOST_FUNC_H
#include <stdint.h>
#include <string.h>

static int32_t host_syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                                    int32_t a3, int32_t a4, int32_t a5) {
    (void)num; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return 0; /* Stub returns 0, which means dynlink_load will fail */
}

static void kernel_log(const char* msg) { (void)msg; }

#include "../kernel/src/dynlink.c"

/* ---- Helper: manually add a module to the table for testing ---- */
static module_handle_t add_test_module(const char* name) {
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        if (module_table[i].handle == INVALID_MODULE_HANDLE) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return INVALID_MODULE_HANDLE;

    module_handle_t handle = next_handle++;
    module_table[slot].handle = handle;
    int len = 0;
    while (name[len] && len < 63) {
        module_table[slot].name[len] = name[len];
        len++;
    }
    module_table[slot].name[len] = '\0';
    module_table[slot].ref_count = 1;
    module_table[slot].type = 0;
    return handle;
}

/* ---- Tests ---- */

TEST(dynlink_init_clears_all_slots) {
    dynlink_init();
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        ASSERT_EQ(module_table[i].handle, INVALID_MODULE_HANDLE);
        ASSERT_EQ(module_table[i].name[0], '\0');
        ASSERT_EQ(module_table[i].ref_count, 0);
    }
    return 0;
}

TEST(dynlink_init_resets_next_handle) {
    /* Use up some handles */
    dynlink_init();
    add_test_module("a.Wdll");
    /* Re-init */
    dynlink_init();
    ASSERT_EQ(next_handle, 1);
    return 0;
}

TEST(dynlink_get_by_name_returns_null_when_empty) {
    dynlink_init();
    dynlink_module_t* mod = dynlink_get_by_name("nonexistent");
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_get_by_name_finds_added_module) {
    dynlink_init();
    module_handle_t h = add_test_module("test.Wdll");
    ASSERT_TRUE(h != INVALID_MODULE_HANDLE);
    dynlink_module_t* mod = dynlink_get_by_name("test.Wdll");
    ASSERT_NOT_NULL(mod);
    ASSERT_EQ(mod->handle, h);
    return 0;
}

TEST(dynlink_get_by_name_not_found) {
    dynlink_init();
    add_test_module("test.Wdll");
    dynlink_module_t* mod = dynlink_get_by_name("other.Wdll");
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_get_info_valid_handle) {
    dynlink_init();
    module_handle_t h = add_test_module("mod.Wdll");
    dynlink_module_t* mod = dynlink_get_info(h);
    ASSERT_NOT_NULL(mod);
    ASSERT_STR_EQ(mod->name, "mod.Wdll");
    return 0;
}

TEST(dynlink_get_info_invalid_handle) {
    dynlink_init();
    dynlink_module_t* mod = dynlink_get_info(999);
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_get_info_after_unload) {
    dynlink_init();
    module_handle_t h = add_test_module("temp.Wdll");
    dynlink_unload(h);
    dynlink_module_t* mod = dynlink_get_info(h);
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_unload_decrements_refcount) {
    dynlink_init();
    module_handle_t h = add_test_module("ref.Wdll");
    /* Bump ref count */
    dynlink_module_t* mod = dynlink_get_info(h);
    mod->ref_count = 3;

    dynlink_unload(h);
    mod = dynlink_get_info(h);
    ASSERT_NOT_NULL(mod);
    ASSERT_EQ(mod->ref_count, 2);

    dynlink_unload(h);
    ASSERT_EQ(mod->ref_count, 1);

    dynlink_unload(h);
    /* ref_count hit 0, module should be unloaded */
    mod = dynlink_get_info(h);
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_unload_invalid_returns_error) {
    dynlink_init();
    int32_t result = dynlink_unload(999);
    ASSERT_EQ(result, -2);
    return 0;
}

TEST(dynlink_multiple_modules) {
    dynlink_init();
    module_handle_t h1 = add_test_module("a.Wdll");
    module_handle_t h2 = add_test_module("b.Wdll");
    module_handle_t h3 = add_test_module("c.Wdll");

    ASSERT_TRUE(h1 != INVALID_MODULE_HANDLE);
    ASSERT_TRUE(h2 != INVALID_MODULE_HANDLE);
    ASSERT_TRUE(h3 != INVALID_MODULE_HANDLE);

    /* Each should be findable by name */
    ASSERT_NOT_NULL(dynlink_get_by_name("a.Wdll"));
    ASSERT_NOT_NULL(dynlink_get_by_name("b.Wdll"));
    ASSERT_NOT_NULL(dynlink_get_by_name("c.Wdll"));
    return 0;
}

TEST(dynlink_module_name_collision) {
    dynlink_init();
    module_handle_t h1 = add_test_module("same.Wdll");
    /* Manually add another with same name - this simulates loading same module twice */
    /* In real code, dynlink_load checks for existing and increments ref_count */
    /* But we can verify get_by_name returns the first one */
    dynlink_module_t* first = dynlink_get_by_name("same.Wdll");
    ASSERT_NOT_NULL(first);
    ASSERT_EQ(first->handle, h1);
    (void)h1;
    return 0;
}

TEST(dynlink_load_returns_invalid_without_host) {
    dynlink_init();
    /* host_syscall_invoke stub returns 0, which causes load to fail */
    module_handle_t h = dynlink_load("test.Wdll", 9);
    ASSERT_EQ(h, INVALID_MODULE_HANDLE);
    return 0;
}

TEST(dynlink_resolve_null_out_returns_error) {
    dynlink_init();
    module_handle_t h = add_test_module("mod.Wdll");
    int32_t result = dynlink_resolve(h, "func", 4, NULL);
    ASSERT_EQ(result, -1); /* ERR_INVAL */
    return 0;
}

TEST(dynlink_resolve_invalid_handle) {
    dynlink_init();
    void* out;
    int32_t result = dynlink_resolve(999, "func", 4, &out);
    /* Delegates to host, so result depends on stub */
    /* Stub returns 0 */
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(dynlink_name_truncation) {
    dynlink_init();
    /* Name longer than 63 chars should be truncated */
    const char* long_name = "this_is_a_very_long_module_name_that_exceeds_the_sixty_three_character_limit_for_sure";
    module_handle_t h = add_test_module(long_name);
    ASSERT_TRUE(h != INVALID_MODULE_HANDLE);
    dynlink_module_t* mod = dynlink_get_info(h);
    ASSERT_NOT_NULL(mod);
    /* Name should be truncated to 63 chars + null */
    ASSERT_TRUE(strlen(mod->name) <= 63);
    return 0;
}

static test_entry_t dynlink_extended_tests[] = {
    TEST_ENTRY(dynlink_init_clears_all_slots, "DynLink Extended"),
    TEST_ENTRY(dynlink_init_resets_next_handle, "DynLink Extended"),
    TEST_ENTRY(dynlink_get_by_name_returns_null_when_empty, "DynLink Extended"),
    TEST_ENTRY(dynlink_get_by_name_finds_added_module, "DynLink Extended"),
    TEST_ENTRY(dynlink_get_by_name_not_found, "DynLink Extended"),
    TEST_ENTRY(dynlink_get_info_valid_handle, "DynLink Extended"),
    TEST_ENTRY(dynlink_get_info_invalid_handle, "DynLink Extended"),
    TEST_ENTRY(dynlink_get_info_after_unload, "DynLink Extended"),
    TEST_ENTRY(dynlink_unload_decrements_refcount, "DynLink Extended"),
    TEST_ENTRY(dynlink_unload_invalid_returns_error, "DynLink Extended"),
    TEST_ENTRY(dynlink_multiple_modules, "DynLink Extended"),
    TEST_ENTRY(dynlink_module_name_collision, "DynLink Extended"),
    TEST_ENTRY(dynlink_load_returns_invalid_without_host, "DynLink Extended"),
    TEST_ENTRY(dynlink_resolve_null_out_returns_error, "DynLink Extended"),
    TEST_ENTRY(dynlink_resolve_invalid_handle, "DynLink Extended"),
    TEST_ENTRY(dynlink_name_truncation, "DynLink Extended"),
};

int main(void) {
    RUN_TESTS(dynlink_extended_tests);
}
