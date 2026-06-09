#ifndef WM_CLIENT_H
#define WM_CLIENT_H

/**
 * Window Manager Client Library
 * 
 * This header is included by user applications that want to
 * create and manage windows. The implementation (wm_client.c)
 * communicates with the window manager service via IPC.
 */

/* Event types */
#define WM_EVENT_CLOSE      1
#define WM_EVENT_RESIZE     2
#define WM_EVENT_KEY_PRESS  3
#define WM_EVENT_KEY_RELEASE 4
#define WM_EVENT_MOUSE_DOWN 5
#define WM_EVENT_MOUSE_UP   6
#define WM_EVENT_MOUSE_MOVE 7

typedef struct {
    int type;
    int window_id;
    int key;
    int x, y;
    int width, height;
    int buttons;
} wm_event_t;

/* Create a window, returns window ID */
int wm_create_window(const char* title, int x, int y, int width, int height);

/* Show/hide a window */
void wm_show_window(int window_id);
void wm_hide_window(int window_id);

/* Move/resize a window */
void wm_move_window(int window_id, int x, int y);
void wm_resize_window(int window_id, int width, int height);

/* Destroy a window */
void wm_destroy_window(int window_id);

/* Wait for the next event (blocking) */
wm_event_t wm_wait_event(int window_id);

/* Poll for an event (non-blocking, returns 1 if event available) */
int wm_poll_event(int window_id, wm_event_t* event);

#endif /* WM_CLIENT_H */
