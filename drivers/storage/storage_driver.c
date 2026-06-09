/**
 * WebOS Storage Driver (.Wdll)
 * 
 * Encapsulates IndexedDB/localStorage as a persistent storage backend.
 * Provides file-like read/write/exists/delete operations.
 */

#include "storage_driver.h"
#include <emscripten.h>

/* JS host functions */
extern int js_fs_read(const char* path, void* buf, int len);
extern int js_fs_write(const char* path, const void* buf, int len);
extern int js_fs_exists(const char* path);
extern int js_fs_mkdir(const char* path);
extern int js_fs_unlink(const char* path);
extern int js_fs_listdir(const char* path, void* result);

EMSCRIPTEN_KEEPALIVE
int storage_driver_init(void) {
    /* Ensure base directories exist */
    js_fs_mkdir("/");
    js_fs_mkdir("/home");
    js_fs_mkdir("/etc");
    js_fs_mkdir("/tmp");
    js_fs_mkdir("/apps");
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int storage_driver_read(const char* path, void* buf, int buf_len) {
    return js_fs_read(path, buf, buf_len);
}

EMSCRIPTEN_KEEPALIVE
int storage_driver_write(const char* path, const void* data, int data_len) {
    return js_fs_write(path, data, data_len);
}

EMSCRIPTEN_KEEPALIVE
int storage_driver_exists(const char* path) {
    return js_fs_exists(path);
}

EMSCRIPTEN_KEEPALIVE
int storage_driver_delete(const char* path) {
    return js_fs_unlink(path);
}

EMSCRIPTEN_KEEPALIVE
int storage_driver_mkdir(const char* path) {
    return js_fs_mkdir(path);
}
