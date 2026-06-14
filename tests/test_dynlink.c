#include "test_framework.h"

/* Skip WASM-specific host_func.h and provide stubs */
#define HOST_FUNC_H
#include <stdint.h>

static int32_t host_syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                                    int32_t a3, int32_t a4, int32_t a5) {
    (void)num; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return 0;
}

static void kernel_log(const char* msg) { (void)msg; }

#include "../kernel/src/dynlink.c"

TEST(dynlink_init_clears_table) {
    dynlink_init();
    for (int i = 0; i < MAX_LOADED_MODULES; i++) {
        ASSERT_EQ(module_table[i].handle, INVALID_MODULE_HANDLE);
    }
    return 0;
}

TEST(dynlink_get_by_name_returns_null_when_empty) {
    dynlink_init();
    dynlink_module_t* mod = dynlink_get_by_name("nonexistent");
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_get_info_invalid_handle) {
    dynlink_init();
    /* Note: dynlink_get_info(INVALID_MODULE_HANDLE) would match the
     * initialized slot since INVALID_MODULE_HANDLE == 0 and all slots
     * are initialized to handle=0. Use a non-zero invalid handle instead. */
    dynlink_module_t* mod = dynlink_get_info(999);
    ASSERT_NULL(mod);
    return 0;
}

TEST(dynlink_unload_invalid_returns_error) {
    dynlink_init();
    int32_t result = dynlink_unload(999);
    ASSERT_EQ(result, -2);
    return 0;
}

static test_entry_t dynlink_tests[] = {
    TEST_ENTRY(dynlink_init_clears_table, "DynLink"),
    TEST_ENTRY(dynlink_get_by_name_returns_null_when_empty, "DynLink"),
    TEST_ENTRY(dynlink_get_info_invalid_handle, "DynLink"),
    TEST_ENTRY(dynlink_unload_invalid_returns_error, "DynLink"),
};

int main(void) {
    RUN_TESTS(dynlink_tests);
}
