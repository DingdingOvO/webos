#ifndef WM_H
#define WM_H

int wm_service_init(void);
int wm_create_window(int owner_pid, const char* title, int x, int y, int w, int h);
void wm_destroy_window(int window_id);
void wm_show_window(int window_id);
void wm_hide_window(int window_id);
void wm_move_window(int window_id, int x, int y);
void wm_resize_window(int window_id, int w, int h);
void wm_set_focus(int window_id);
int wm_get_focused_window(void);
int wm_get_window_count(void);
void wm_render_all(int surface);

#endif
