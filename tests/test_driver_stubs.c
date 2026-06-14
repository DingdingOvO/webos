/**
 * WebOS Driver API Stub Tests
 *
 * Tests that verify driver API contracts by exercising the driver code
 * with stubbed-out JS host functions. All four driver modules are
 * included after defining the necessary JS function stubs.
 *
 * Compile with: -Istubs -I../drivers/gpu -I../drivers/input
 *               -I../drivers/storage -I../drivers/network
 */

#include "test_framework.h"
#include <stdint.h>
#include <string.h>

/* ---- Stub JS host functions ---- */
int js_gpu_init(void) { return 1; }
int js_gpu_create_surface(int w, int h) { (void)w; (void)h; return 1; }
void js_gpu_draw_rect(int s, int x, int y, int w, int h, unsigned int c)
    { (void)s; (void)x; (void)y; (void)w; (void)h; (void)c; }
void js_gpu_draw_text(int s, int x, int y, const char* t, int c)
    { (void)s; (void)x; (void)y; (void)t; (void)c; }
void js_gpu_clear(int s, unsigned int c) { (void)s; (void)c; }
void js_gpu_present(int s) { (void)s; }
void js_gpu_destroy_surface(int s) { (void)s; }
int js_gpu_get_width(void) { return 1024; }
int js_gpu_get_height(void) { return 768; }

void js_input_subscribe(int cb) { (void)cb; }
int js_input_get_event(void* ev) { (void)ev; return 0; }

int js_fs_read(const char* p, void* b, int l) { (void)p; (void)b; (void)l; return 0; }
int js_fs_write(const char* p, const void* b, int l) { (void)p; (void)b; (void)l; return l; }
int js_fs_exists(const char* p) { (void)p; return 0; }
int js_fs_mkdir(const char* p) { (void)p; return 0; }
int js_fs_unlink(const char* p) { (void)p; return 0; }
int js_fs_listdir(const char* p, void* r) { (void)p; (void)r; return 0; }

int js_net_fetch(const char* url, const char* method, const void* body,
                 int body_len, void* resp) {
    (void)url; (void)method; (void)body; (void)body_len; (void)resp;
    return 0;
}

/* Include driver source files (emscripten.h stub found via -Istubs) */
#include "../../drivers/gpu/gpu_driver.c"
#include "../../drivers/input/input_driver.c"
#include "../../drivers/storage/storage_driver.c"
#include "../../drivers/network/network_driver.c"

/* ============ GPU Driver Tests ============ */

TEST(gpu_init_returns_zero) {
    gpu_initialized = 0;
    int result = gpu_driver_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(gpu_init_sets_initialized_flag) {
    gpu_initialized = 0;
    gpu_driver_init();
    ASSERT_EQ(gpu_initialized, 1);
    return 0;
}

TEST(gpu_init_idempotent) {
    gpu_initialized = 0;
    gpu_driver_init();
    int result = gpu_driver_init();
    ASSERT_EQ(result, 0);
    ASSERT_EQ(gpu_initialized, 1);
    return 0;
}

TEST(gpu_screen_dimensions_after_init) {
    gpu_initialized = 0;
    gpu_driver_init();
    ASSERT_EQ(gpu_driver_get_screen_width(), 1024);
    ASSERT_EQ(gpu_driver_get_screen_height(), 768);
    return 0;
}

TEST(gpu_create_surface_returns_positive) {
    gpu_initialized = 0;
    gpu_driver_init();
    int s = gpu_driver_create_surface(800, 600);
    ASSERT_TRUE(s > 0);
    return 0;
}

TEST(gpu_default_surface_after_init) {
    gpu_initialized = 0;
    gpu_driver_init();
    int ds = gpu_driver_get_default_surface();
    ASSERT_TRUE(ds >= 0);
    return 0;
}

/* ============ Input Driver Tests ============ */

TEST(input_init_clears_queue) {
    input_driver_init();
    ASSERT_EQ(queue_head, 0);
    ASSERT_EQ(queue_tail, 0);
    ASSERT_EQ(queue_count, 0);
    return 0;
}

TEST(input_push_then_poll_event) {
    input_driver_init();
    input_event_t ev = { .type = INPUT_EVENT_KEY_DOWN, .key = 65, .x = 0, .y = 0, .buttons = 0, .delta = 0 };
    input_driver_push_event(&ev);
    ASSERT_EQ(queue_count, 1);
    ASSERT_EQ(input_driver_has_events(), 1);

    input_event_t out;
    int got = input_driver_poll_event(&out);
    ASSERT_EQ(got, 1);
    ASSERT_EQ(out.type, INPUT_EVENT_KEY_DOWN);
    ASSERT_EQ(out.key, 65);
    ASSERT_EQ(queue_count, 0);
    return 0;
}

TEST(input_has_events_empty) {
    input_driver_init();
    ASSERT_EQ(input_driver_has_events(), 0);
    return 0;
}

TEST(input_poll_empty_returns_zero) {
    input_driver_init();
    input_event_t ev;
    int got = input_driver_poll_event(&ev);
    ASSERT_EQ(got, 0);
    return 0;
}

TEST(input_push_multiple_fifo_order) {
    input_driver_init();
    input_event_t ev1 = { .type = INPUT_EVENT_KEY_DOWN, .key = 65, .x = 0, .y = 0, .buttons = 0, .delta = 0 };
    input_event_t ev2 = { .type = INPUT_EVENT_KEY_UP, .key = 65, .x = 0, .y = 0, .buttons = 0, .delta = 0 };
    input_event_t ev3 = { .type = INPUT_EVENT_MOUSE_DOWN, .key = 0, .x = 100, .y = 200, .buttons = 1, .delta = 0 };
    input_driver_push_event(&ev1);
    input_driver_push_event(&ev2);
    input_driver_push_event(&ev3);
    ASSERT_EQ(queue_count, 3);

    input_event_t out;
    input_driver_poll_event(&out);
    ASSERT_EQ(out.type, INPUT_EVENT_KEY_DOWN);
    input_driver_poll_event(&out);
    ASSERT_EQ(out.type, INPUT_EVENT_KEY_UP);
    input_driver_poll_event(&out);
    ASSERT_EQ(out.type, INPUT_EVENT_MOUSE_DOWN);
    ASSERT_EQ(out.x, 100);
    ASSERT_EQ(out.y, 200);
    return 0;
}

TEST(input_event_types_values) {
    ASSERT_EQ(INPUT_EVENT_KEY_DOWN, 1);
    ASSERT_EQ(INPUT_EVENT_KEY_UP, 2);
    ASSERT_EQ(INPUT_EVENT_MOUSE_DOWN, 3);
    ASSERT_EQ(INPUT_EVENT_MOUSE_UP, 4);
    ASSERT_EQ(INPUT_EVENT_MOUSE_MOVE, 5);
    ASSERT_EQ(INPUT_EVENT_WHEEL, 6);
    return 0;
}

/* ============ Storage Driver Tests ============ */

TEST(storage_init_returns_zero) {
    int result = storage_driver_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(storage_write_returns_length) {
    const char* data = "hello";
    int result = storage_driver_write("/tmp/test.txt", data, 5);
    ASSERT_EQ(result, 5);
    return 0;
}

TEST(storage_exists_returns_stub_value) {
    int result = storage_driver_exists("/tmp/nonexistent.txt");
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(storage_mkdir_returns_stub_value) {
    int result = storage_driver_mkdir("/tmp/testdir");
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(storage_delete_returns_stub_value) {
    int result = storage_driver_delete("/tmp/test.txt");
    ASSERT_EQ(result, 0);
    return 0;
}

/* ============ Network Driver Tests ============ */

TEST(network_init_returns_zero) {
    int result = network_driver_init();
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(network_http_get_delegates) {
    char buf[256];
    int result = network_driver_http_get("http://example.com", buf, 256);
    ASSERT_EQ(result, 0);
    return 0;
}

TEST(network_http_post_delegates) {
    const char* body = "data";
    char buf[256];
    int result = network_driver_http_post("http://example.com", body, 4, buf);
    ASSERT_EQ(result, 0);
    return 0;
}

static test_entry_t driver_stub_tests[] = {
    TEST_ENTRY(gpu_init_returns_zero, "GPU Driver"),
    TEST_ENTRY(gpu_init_sets_initialized_flag, "GPU Driver"),
    TEST_ENTRY(gpu_init_idempotent, "GPU Driver"),
    TEST_ENTRY(gpu_screen_dimensions_after_init, "GPU Driver"),
    TEST_ENTRY(gpu_create_surface_returns_positive, "GPU Driver"),
    TEST_ENTRY(gpu_default_surface_after_init, "GPU Driver"),
    TEST_ENTRY(input_init_clears_queue, "Input Driver"),
    TEST_ENTRY(input_push_then_poll_event, "Input Driver"),
    TEST_ENTRY(input_has_events_empty, "Input Driver"),
    TEST_ENTRY(input_poll_empty_returns_zero, "Input Driver"),
    TEST_ENTRY(input_push_multiple_fifo_order, "Input Driver"),
    TEST_ENTRY(input_event_types_values, "Input Driver"),
    TEST_ENTRY(storage_init_returns_zero, "Storage Driver"),
    TEST_ENTRY(storage_write_returns_length, "Storage Driver"),
    TEST_ENTRY(storage_exists_returns_stub_value, "Storage Driver"),
    TEST_ENTRY(storage_mkdir_returns_stub_value, "Storage Driver"),
    TEST_ENTRY(storage_delete_returns_stub_value, "Storage Driver"),
    TEST_ENTRY(network_init_returns_zero, "Network Driver"),
    TEST_ENTRY(network_http_get_delegates, "Network Driver"),
    TEST_ENTRY(network_http_post_delegates, "Network Driver"),
};

int main(void) {
    RUN_TESTS(driver_stub_tests);
}
