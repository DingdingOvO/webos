/**
 * WebOS Network Driver (.Wdll)
 * 
 * Encapsulates the browser's Fetch API as a network backend.
 * Provides HTTP request capabilities for higher-level services.
 */

#include "network_driver.h"
#include <emscripten.h>

/* JS host functions */
extern int js_net_fetch(const char* url, const char* method, const void* body, int body_len, void* response);

EMSCRIPTEN_KEEPALIVE
int network_driver_init(void) {
    return 0; /* No special initialization needed */
}

EMSCRIPTEN_KEEPALIVE
int network_driver_http_get(const char* url, void* response_buf, int buf_len) {
    return js_net_fetch(url, "GET", 0, 0, response_buf);
}

EMSCRIPTEN_KEEPALIVE
int network_driver_http_post(const char* url, const void* body, int body_len, void* response_buf) {
    return js_net_fetch(url, "POST", body, body_len, response_buf);
}
