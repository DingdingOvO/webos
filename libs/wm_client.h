#ifndef WM_CLIENT_H
#define WM_CLIENT_H

/**
 * WebOS Window Manager Client Interface
 * 
 * Client-side API for interacting with the window manager service.
 * Applications use these functions to create, manipulate, and
 * render to windows.
 */

#include <stdint.h>

/* Window handle */
typedef int window_handle_t;
#define INVALID_WINDOW 0

/* Window creation flags */
#define WM_FLAG_RESIZABLE   0x01
#define WM_FLAG_MOVABLE     0x02
#define WM_FLAG_CLOSABLE    0x04
#define WM_FLAG_MINIMIZABLE 0x08
#define WM_FLAG_BORDERLESS  0x10
#define WM_FLAG_FULLSCREEN  0x20

/* Window event types */
#define WM_EVENT_CLOSE      1
#define WM_EVENT_RESIZE     2
#define WM_EVENT_MOVE       3
#define WM_EVENT_FOCUS      4
#define WM_EVENT_BLUR       5
#define WM_EVENT_MOUSE      6
#define WM_EVENT_KEY        7

/* Mouse event subtypes */
#define WM_MOUSE_MOVE       0
#define WM_MOUSE_DOWN       1
#define WM_MOUSE_UP         2
#define WM_MOUSE_WHEEL      3

/* Key event subtypes */
#define WM_KEY_DOWN         0
#define WM_KEY_UP           1

/* Window rectangle */
typedef struct {
    int x, y;
    int width, height;
} wm_rect_t;

/* Window event structure */
typedef struct {
    int type;
    window_handle_t window;
    union {
        struct { int width, height; } resize;
        struct { int x, y; } move;
        struct { unsigned int button; int x, y; unsigned int subtype; } mouse;
        struct { unsigned int keycode; unsigned int modifiers; unsigned int subtype; } key;
    } data;
} wm_event_t;

/* Pixel format: RGBA8888 */
typedef struct {
    uint8_t r, g, b, a;
} wm_pixel_t;

/**
 * Create a new window.
 * 
 * @param title   Window title string
 * @param x, y    Window position
 * @param width, height  Window dimensions
 * @return        Window handle, or <= 0 on error
 */
int wm_create_window(const char* title, int x, int y, int width, int height);

/**
 * Show a window.
 */
void wm_show_window(int window_id);

/**
 * Hide a window.
 */
void wm_hide_window(int window_id);

/**
 * Move a window to new position.
 */
void wm_move_window(int window_id, int x, int y);

/**
 * Resize a window.
 */
void wm_resize_window(int window_id, int width, int height);

/**
 * Destroy a window.
 */
void wm_destroy_window(int window_id);

/**
 * Wait for the next event (blocking).
 */
wm_event_t wm_wait_event(int window_id);

/**
 * Poll for events (non-blocking).
 * Returns 0 if no events, 1 if event available.
 */
int wm_poll_event(int window_id, wm_event_t* event);

#endif /* WM_CLIENT_H */
