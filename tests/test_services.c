/**
 * WebOS Service API Tests
 *
 * Tests for window manager, filesystem service, network service, and app store.
 * Driver dependencies are stubbed out.
 *
 * Compile with: -Istubs -I../services/window_manager -I../services/filesystem
 *               -I../services/network_service -I../services/appstore
 */

#include "test_framework.h"
#include <stdint.h>
#include <string.h>

/* EMSCRIPTEN_KEEPALIVE is not defined in regular gcc - define it as nothing */
#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

/* ---- Stub driver functions (satisfy extern declarations in services) ---- */
int storage_driver_read(const char* p, void* b, int l) { (void)p; (void)b; (void)l; return 0; }
int storage_driver_write(const char* p, const void* b, int l) { (void)p; (void)b; (void)l; return l; }
int storage_driver_mkdir(const char* p) { (void)p; return 0; }
int storage_driver_delete(const char* p) { (void)p; return 0; }
int storage_driver_exists(const char* p) { (void)p; return 0; }
int network_driver_http_get(const char* url, void* buf, int len) { (void)url; (void)buf; (void)len; return 0; }
int network_driver_http_post(const char* url, const void* body, int blen, void* buf) {
    (void)url; (void)body; (void)blen; (void)buf; return 0;
}

/* Include service source files */
#include "../../services/window_manager/wm.c"
#include "../../services/filesystem/fs_service.c"
#include "../../services/network_service/net_service.c"
#include "../../services/appstore/appstore.c"

/* ============ Window Manager Tests ============ */

TEST(wm_init_returns_zero) {
    int result = wm_service_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(wm_create_window_returns_id) {
    wm_service_init();
    int wid = wm_create_window(1, "Test Window", 10, 20, 300, 200);
    ASSERT_TRUE(wid > 0);
    return 0;
}

TEST(wm_create_multiple_windows) {
    wm_service_init();
    int w1 = wm_create_window(1, "Win1", 0, 0, 100, 100);
    int w2 = wm_create_window(2, "Win2", 100, 100, 200, 200);
    int w3 = wm_create_window(3, "Win3", 200, 200, 300, 300);
    ASSERT_TRUE(w1 > 0);
    ASSERT_TRUE(w2 > 0);
    ASSERT_TRUE(w3 > 0);
    ASSERT_TRUE(w2 != w1);
    ASSERT_TRUE(w3 != w1);
    ASSERT_TRUE(w3 != w2);
    return 0;
}

TEST(wm_window_count) {
    wm_service_init();
    ASSERT_EQ(wm_get_window_count(), 0);
    wm_create_window(1, "W1", 0, 0, 100, 100);
    ASSERT_EQ(wm_get_window_count(), 1);
    wm_create_window(2, "W2", 0, 0, 100, 100);
    ASSERT_EQ(wm_get_window_count(), 2);
    return 0;
}

TEST(wm_destroy_window) {
    wm_service_init();
    int wid = wm_create_window(1, "DestroyMe", 0, 0, 100, 100);
    ASSERT_EQ(wm_get_window_count(), 1);
    wm_destroy_window(wid);
    ASSERT_EQ(wm_get_window_count(), 0);
    return 0;
}

TEST(wm_move_window) {
    wm_service_init();
    int wid = wm_create_window(1, "Movable", 10, 20, 100, 100);
    wm_move_window(wid, 50, 60);
    /* Verify by checking the window state - we need to find it */
    int found = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == wid) {
            ASSERT_EQ(windows[i].x, 50);
            ASSERT_EQ(windows[i].y, 60);
            found = 1;
        }
    }
    ASSERT_TRUE(found);
    return 0;
}

TEST(wm_resize_window) {
    wm_service_init();
    int wid = wm_create_window(1, "Resizable", 0, 0, 100, 100);
    wm_resize_window(wid, 400, 300);
    int found = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == wid) {
            ASSERT_EQ(windows[i].width, 400);
            ASSERT_EQ(windows[i].height, 300);
            found = 1;
        }
    }
    ASSERT_TRUE(found);
    return 0;
}

TEST(wm_set_focus) {
    wm_service_init();
    int w1 = wm_create_window(1, "W1", 0, 0, 100, 100);
    int w2 = wm_create_window(2, "W2", 0, 0, 100, 100);
    wm_set_focus(w1);
    ASSERT_EQ(wm_get_focused_window(), w1);
    wm_set_focus(w2);
    ASSERT_EQ(wm_get_focused_window(), w2);
    return 0;
}

TEST(wm_destroy_focused_resets_focus) {
    wm_service_init();
    int wid = wm_create_window(1, "Focus", 0, 0, 100, 100);
    wm_set_focus(wid);
    ASSERT_EQ(wm_get_focused_window(), wid);
    wm_destroy_window(wid);
    ASSERT_EQ(wm_get_focused_window(), 0);
    return 0;
}

TEST(wm_show_hide_window) {
    wm_service_init();
    int wid = wm_create_window(1, "Visible", 0, 0, 100, 100);
    wm_hide_window(wid);
    int found = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == wid) {
            ASSERT_EQ(windows[i].visible, 0);
            found = 1;
        }
    }
    ASSERT_TRUE(found);
    wm_show_window(wid);
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == wid) {
            ASSERT_EQ(windows[i].visible, 1);
        }
    }
    return 0;
}

TEST(wm_render_all_no_crash) {
    wm_service_init();
    wm_create_window(1, "W1", 0, 0, 100, 100);
    wm_create_window(2, "W2", 0, 0, 100, 100);
    /* Should not crash */
    wm_render_all(0);
    return 0;
}

/* ============ Filesystem Service Tests ============ */

TEST(fs_init_returns_zero) {
    int result = fs_service_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(fs_open_returns_fd) {
    fs_service_init();
    int fd = fs_open(1, "/tmp/test.txt", O_RDWR | O_CREAT);
    ASSERT_TRUE(fd > 0);
    return 0;
}

TEST(fs_close_returns_zero) {
    fs_service_init();
    int fd = fs_open(1, "/tmp/test.txt", O_RDWR | O_CREAT);
    int result = fs_close(fd);
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(fs_close_invalid_fd_returns_error) {
    fs_service_init();
    int result = fs_close(999);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(fs_open_increments_fd) {
    fs_service_init();
    int fd1 = fs_open(1, "/tmp/a.txt", O_RDONLY);
    int fd2 = fs_open(1, "/tmp/b.txt", O_RDONLY);
    ASSERT_TRUE(fd2 > fd1);
    return 0;
}

TEST(fs_mkdir_returns_stub) {
    fs_service_init();
    int result = fs_mkdir("/tmp/newdir");
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(fs_exists_returns_stub) {
    fs_service_init();
    int result = fs_exists("/tmp/anything");
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(fs_unlink_returns_stub) {
    fs_service_init();
    int result = fs_unlink("/tmp/file.txt");
    ASSERT_EQ(result, 0);
    return 0;
}

/* ============ Network Service Tests ============ */

TEST(net_init_returns_zero) {
    int result = net_service_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(net_open_connection_returns_id) {
    net_service_init();
    int conn = net_open_connection(1, "ws://localhost:8080");
    ASSERT_TRUE(conn > 0);
    return 0;
}

TEST(net_close_connection) {
    net_service_init();
    int conn = net_open_connection(1, "ws://localhost:8080");
    net_close_connection(conn);
    /* Verify connection is closed */
    int found = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].id == conn) {
            ASSERT_EQ(connections[i].state, 0);
            found = 1;
        }
    }
    ASSERT_TRUE(found);
    return 0;
}

TEST(net_http_get_delegates) {
    net_service_init();
    char buf[256];
    int result = net_http_get("http://example.com", buf, 256);
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(net_http_post_delegates) {
    net_service_init();
    const char* body = "data";
    char buf[256];
    int result = net_http_post("http://example.com", body, 4, buf);
    ASSERT_EQ(result, 0);
    return 0;
}

/* ============ App Store Tests ============ */

TEST(appstore_init_returns_zero) {
    int result = appstore_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(appstore_init_registers_builtin_apps) {
    appstore_init();
    ASSERT_TRUE(appstore_get_count() >= 4);
    return 0;
}

TEST(appstore_register_app) {
    appstore_init();
    int count_before = appstore_get_count();
    int id = appstore_register("MyApp", "A test app", "1.0.0",
                                "/apps/myapp.wex", "/icons/myapp.png", "Test");
    ASSERT_TRUE(id > 0);
    ASSERT_EQ(appstore_get_count(), count_before + 1);
    return 0;
}

TEST(appstore_find_by_name) {
    appstore_init();
    int id = appstore_find_by_name("Calculator");
    ASSERT_TRUE(id > 0);
    return 0;
}

TEST(appstore_find_by_name_not_found) {
    appstore_init();
    int id = appstore_find_by_name("NonExistentApp");
    ASSERT_EQ(id, -1);
    return 0;
}

TEST(appstore_install_and_check) {
    appstore_init();
    int id = appstore_find_by_name("Calculator");
    ASSERT_EQ(appstore_is_installed(id), 0);
    int result = appstore_install(id);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(appstore_is_installed(id), 1);
    return 0;
}

TEST(appstore_uninstall) {
    appstore_init();
    int id = appstore_find_by_name("Calculator");
    appstore_install(id);
    ASSERT_EQ(appstore_is_installed(id), 1);
    int result = appstore_uninstall(id);
    ASSERT_EQ(result, 0);
    ASSERT_EQ(appstore_is_installed(id), 0);
    return 0;
}

TEST(appstore_install_invalid_returns_error) {
    appstore_init();
    int result = appstore_install(9999);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(appstore_uninstall_invalid_returns_error) {
    appstore_init();
    int result = appstore_uninstall(9999);
    ASSERT_EQ(result, -1);
    return 0;
}

TEST(appstore_is_installed_invalid_returns_zero) {
    appstore_init();
    int result = appstore_is_installed(9999);
    ASSERT_EQ(result, 0);
    return 0;
}

static test_entry_t services_tests[] = {
    TEST_ENTRY(wm_init_returns_zero, "Window Manager"),
    TEST_ENTRY(wm_create_window_returns_id, "Window Manager"),
    TEST_ENTRY(wm_create_multiple_windows, "Window Manager"),
    TEST_ENTRY(wm_window_count, "Window Manager"),
    TEST_ENTRY(wm_destroy_window, "Window Manager"),
    TEST_ENTRY(wm_move_window, "Window Manager"),
    TEST_ENTRY(wm_resize_window, "Window Manager"),
    TEST_ENTRY(wm_set_focus, "Window Manager"),
    TEST_ENTRY(wm_destroy_focused_resets_focus, "Window Manager"),
    TEST_ENTRY(wm_show_hide_window, "Window Manager"),
    TEST_ENTRY(wm_render_all_no_crash, "Window Manager"),
    TEST_ENTRY(fs_init_returns_zero, "Filesystem Service"),
    TEST_ENTRY(fs_open_returns_fd, "Filesystem Service"),
    TEST_ENTRY(fs_close_returns_zero, "Filesystem Service"),
    TEST_ENTRY(fs_close_invalid_fd_returns_error, "Filesystem Service"),
    TEST_ENTRY(fs_open_increments_fd, "Filesystem Service"),
    TEST_ENTRY(fs_mkdir_returns_stub, "Filesystem Service"),
    TEST_ENTRY(fs_exists_returns_stub, "Filesystem Service"),
    TEST_ENTRY(fs_unlink_returns_stub, "Filesystem Service"),
    TEST_ENTRY(net_init_returns_zero, "Network Service"),
    TEST_ENTRY(net_open_connection_returns_id, "Network Service"),
    TEST_ENTRY(net_close_connection, "Network Service"),
    TEST_ENTRY(net_http_get_delegates, "Network Service"),
    TEST_ENTRY(net_http_post_delegates, "Network Service"),
    TEST_ENTRY(appstore_init_returns_zero, "App Store"),
    TEST_ENTRY(appstore_init_registers_builtin_apps, "App Store"),
    TEST_ENTRY(appstore_register_app, "App Store"),
    TEST_ENTRY(appstore_find_by_name, "App Store"),
    TEST_ENTRY(appstore_find_by_name_not_found, "App Store"),
    TEST_ENTRY(appstore_install_and_check, "App Store"),
    TEST_ENTRY(appstore_uninstall, "App Store"),
    TEST_ENTRY(appstore_install_invalid_returns_error, "App Store"),
    TEST_ENTRY(appstore_uninstall_invalid_returns_error, "App Store"),
    TEST_ENTRY(appstore_is_installed_invalid_returns_zero, "App Store"),
};

int main(void) {
    RUN_TESTS(services_tests);
}
