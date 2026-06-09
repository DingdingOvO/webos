/**
 * WebOS GPU Driver (.Wdll)
 * 
 * Encapsulates WebGPU API as C functions accessible to other modules.
 * This driver is loaded dynamically by the kernel and provides
 * rendering primitives for the window manager and applications.
 */

#include "gpu_driver.h"
#include <emscripten.h>

/* JS host function declarations */
extern int js_gpu_init(void);
extern int js_gpu_create_surface(int width, int height);
extern void js_gpu_draw_rect(int surface, int x, int y, int w, int h, unsigned int color);
extern void js_gpu_draw_text(int surface, int x, int y, const char* text, int color);
extern void js_gpu_clear(int surface, unsigned int color);
extern void js_gpu_present(int surface);
extern void js_gpu_destroy_surface(int surface);
extern int js_gpu_get_width(void);
extern int js_gpu_get_height(void);

/* Global GPU state */
static int gpu_initialized = 0;
static int default_surface = 0;
static int screen_width = 1024;
static int screen_height = 768;

EMSCRIPTEN_KEEPALIVE
int gpu_driver_init(void) {
    if (gpu_initialized) return 0;
    
    int result = js_gpu_init();
    if (result > 0) {
        gpu_initialized = 1;
        screen_width = js_gpu_get_width();
        screen_height = js_gpu_get_height();
        default_surface = js_gpu_create_surface(screen_width, screen_height);
        return 0;
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
int gpu_driver_create_surface(int width, int height) {
    return js_gpu_create_surface(width, height);
}

EMSCRIPTEN_KEEPALIVE
void gpu_driver_draw_rect(int surface, int x, int y, int w, int h, unsigned int color) {
    js_gpu_draw_rect(surface, x, y, w, h, color);
}

EMSCRIPTEN_KEEPALIVE
void gpu_driver_draw_text(int surface, int x, int y, const char* text, unsigned int color) {
    js_gpu_draw_text(surface, x, y, text, color);
}

EMSCRIPTEN_KEEPALIVE
void gpu_driver_clear(int surface, unsigned int color) {
    js_gpu_clear(surface, color);
}

EMSCRIPTEN_KEEPALIVE
void gpu_driver_present(int surface) {
    js_gpu_present(surface);
}

EMSCRIPTEN_KEEPALIVE
void gpu_driver_destroy_surface(int surface) {
    js_gpu_destroy_surface(surface);
}

EMSCRIPTEN_KEEPALIVE
int gpu_driver_get_screen_width(void) {
    return screen_width;
}

EMSCRIPTEN_KEEPALIVE
int gpu_driver_get_screen_height(void) {
    return screen_height;
}

EMSCRIPTEN_KEEPALIVE
int gpu_driver_get_default_surface(void) {
    return default_surface;
}
