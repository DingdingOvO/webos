/**
 * Window Manager Client Library Implementation
 * 
 * Communicates with the WM service via IPC messages.
 */

#include "wm_client.h"
#include "syscall.h"
#include <string.h>

int wm_create_window(const char* title, int x, int y, int width, int height) {
    /* In production: send IPC message to WM service */
    /* Simplified: direct function call (same address space in WASM) */
    extern int wm_create_window_service(int, const char*, int, int, int, int);
    int pid = syscall_getpid();
    return wm_create_window_service(pid, title, x, y, width, height);
}

void wm_show_window(int window_id) {
    extern void wm_show_window_service(int);
    wm_show_window_service(window_id);
}

void wm_hide_window(int window_id) {
    extern void wm_hide_window_service(int);
    wm_hide_window_service(window_id);
}

void wm_move_window(int window_id, int x, int y) {
    extern void wm_move_window_service(int, int, int);
    wm_move_window_service(window_id, x, y);
}

void wm_resize_window(int window_id, int width, int height) {
    extern void wm_resize_window_service(int, int, int);
    wm_resize_window_service(window_id, width, height);
}

void wm_destroy_window(int window_id) {
    extern void wm_destroy_window_service(int);
    wm_destroy_window_service(window_id);
}

wm_event_t wm_wait_event(int window_id) {
    wm_event_t ev;
    memset(&ev, 0, sizeof(ev));
    /* Block until event arrives - poll IPC */
    while (wm_poll_event(window_id, &ev) == 0) {
        /* Yield to scheduler */
    }
    return ev;
}

int wm_poll_event(int window_id, wm_event_t* event) {
    /* Check IPC for events */
    memset(event, 0, sizeof(wm_event_t));
    return 0; /* No events (simplified) */
}
