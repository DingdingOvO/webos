#ifndef GPU_DRIVER_H
#define GPU_DRIVER_H

int gpu_driver_init(void);
int gpu_driver_create_surface(int width, int height);
void gpu_driver_draw_rect(int surface, int x, int y, int w, int h, unsigned int color);
void gpu_driver_draw_text(int surface, int x, int y, const char* text, unsigned int color);
void gpu_driver_clear(int surface, unsigned int color);
void gpu_driver_present(int surface);
void gpu_driver_destroy_surface(int surface);
int gpu_driver_get_screen_width(void);
int gpu_driver_get_screen_height(void);
int gpu_driver_get_default_surface(void);

#endif
