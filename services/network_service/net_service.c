/**
 * WebOS Network Service (.Wdll)
 * 
 * Provides socket-style API on top of the network driver.
 * Supports HTTP GET/POST operations and URL management.
 */

#include "net_service.h"
#include <string.h>

#define MAX_CONNECTIONS 16

typedef struct {
    int id;
    int owner_pid;
    char url[512];
    int state; /* 0=closed, 1=connecting, 2=connected, 3=error */
} net_connection_t;

static net_connection_t connections[MAX_CONNECTIONS];
static int next_conn_id = 1;

EMSCRIPTEN_KEEPALIVE
int net_service_init(void) {
    memset(connections, 0, sizeof(connections));
    next_conn_id = 1;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int net_http_get(const char* url, void* response_buf, int buf_len) {
    extern int network_driver_http_get(const char*, void*, int);
    return network_driver_http_get(url, response_buf, buf_len);
}

EMSCRIPTEN_KEEPALIVE
int net_http_post(const char* url, const void* body, int body_len, void* response_buf) {
    extern int network_driver_http_post(const char*, const void*, int, void*);
    return network_driver_http_post(url, body, body_len, response_buf);
}

EMSCRIPTEN_KEEPALIVE
int net_open_connection(int owner_pid, const char* url) {
    int slot = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].state == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    net_connection_t* conn = &connections[slot];
    conn->id = next_conn_id++;
    conn->owner_pid = owner_pid;
    conn->state = 1;
    
    int len = 0;
    while (url[len] && len < 511) {
        conn->url[len] = url[len];
        len++;
    }
    conn->url[len] = '\0';
    
    return conn->id;
}

EMSCRIPTEN_KEEPALIVE
void net_close_connection(int conn_id) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].id == conn_id) {
            connections[i].state = 0;
            break;
        }
    }
}
