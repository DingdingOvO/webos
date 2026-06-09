/**
 * WebOS Window Manager Service (.Wdll)
 * 
 * Manages windows, Z-ordering, focus, and rendering coordination.
 * Applications use the WM client library (wm_client.h) to create
 * and manipulate windows through IPC messages to this service.
 */

#include "wm.h"
#include <string.h>

#define MAX_WINDOWS 64
#define MAX_TITLE_LEN 128

/* Window structure */
typedef struct {
    int id;
    int owner_pid;
    char title[MAX_TITLE_LEN];
    int x, y, width, height;
    int z_order;
    int visible;
    int minimized;
    int focused;
    unsigned int bg_color;
} wm_window_t;

/* Window manager state */
static wm_window_t windows[MAX_WINDOWS];
static int window_count = 0;
static int next_window_id = 1;
static int focused_window = 0;

EMSCRIPTEN_KEEPALIVE
int wm_service_init(void) {
    memset(windows, 0, sizeof(windows));
    window_count = 0;
    next_window_id = 1;
    focused_window = 0;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int wm_create_window(int owner_pid, const char* title, int x, int y, int w, int h) {
    if (window_count >= MAX_WINDOWS) return -1;
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1;
    
    wm_window_t* win = &windows[slot];
    win->id = next_window_id++;
    win->owner_pid = owner_pid;
    
    /* Copy title */
    int len = 0;
    while (title[len] && len < MAX_TITLE_LEN - 1) {
        win->title[len] = title[len];
        len++;
    }
    win->title[len] = '\0';
    
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->z_order = window_count;
    win->visible = 1;
    win->minimized = 0;
    win->focused = 0;
    win->bg_color = 0xFFFFFFFF;
    
    window_count++;
    return win->id;
}

EMSCRIPTEN_KEEPALIVE
void wm_destroy_window(int window_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == window_id) {
            windows[i].id = 0;
            window_count--;
            if (focused_window == window_id) {
                focused_window = 0;
            }
            break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wm_show_window(int window_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == window_id) {
            windows[i].visible = 1;
            windows[i].minimized = 0;
            break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wm_hide_window(int window_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == window_id) {
            windows[i].visible = 0;
            break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wm_move_window(int window_id, int x, int y) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == window_id) {
            windows[i].x = x;
            windows[i].y = y;
            break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wm_resize_window(int window_id, int w, int h) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == window_id) {
            windows[i].width = w;
            windows[i].height = h;
            break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wm_set_focus(int window_id) {
    /* Unfocus current */
    if (focused_window > 0) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].id == focused_window) {
                windows[i].focused = 0;
                break;
            }
        }
    }
    
    /* Focus new window */
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].id == window_id) {
            windows[i].focused = 1;
            windows[i].z_order = window_count; /* Bring to top */
            focused_window = window_id;
            break;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int wm_get_focused_window(void) {
    return focused_window;
}

EMSCRIPTEN_KEEPALIVE
int wm_get_window_count(void) {
    return window_count;
}

EMSCRIPTEN_KEEPALIVE
void wm_render_all(int surface) {
    /* Sort by z_order and render visible windows */
    /* In production, this calls gpu_driver_draw_rect for each window */
    /* Simplified: just iterate and call draw */
    for (int z = 0; z < window_count; z++) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (windows[i].id > 0 && windows[i].z_order == z && windows[i].visible && !windows[i].minimized) {
                /* Render window frame + content area */
                /* This would call GPU driver functions */
            }
        }
    }
}
