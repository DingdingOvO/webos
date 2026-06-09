/**
 * WebOS Paint Application (.wex)
 * 
 * A simple drawing application with brush, eraser, and color selection.
 * Demonstrates mouse input handling and canvas rendering in WebOS.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <emscripten.h>

extern "C" {
    int wm_create_window(const char* title, int x, int y, int width, int height);
    void wm_show_window(int window_id);
    void wm_destroy_window(int window_id);
    
    #define SYS_exit 0x0200
    long syscall1(long n, long a1);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
}

/* Paint state */
static int current_window = 0;
static int brush_size = 3;
static unsigned int brush_color = 0xFF000000; /* Black */
static int canvas_width = 640;
static int canvas_height = 480;
static int last_x = -1, last_y = -1;
static int drawing = 0;

/* Color palette */
static unsigned int palette[] = {
    0xFF000000, /* Black */
    0xFFFF0000, /* Red */
    0xFF00FF00, /* Green */
    0xFF0000FF, /* Blue */
    0xFFFFFF00, /* Yellow */
    0xFFFF00FF, /* Magenta */
    0xFF00FFFF, /* Cyan */
    0xFFFFFFFF, /* White */
};

void paint_tick(void* arg) {
    /* In production:
     * 1. Poll input events for mouse position
     * 2. If drawing, render line from last_x/last_y to current position
     * 3. Present the canvas surface
     */
}

int main(int argc, char** argv) {
    current_window = wm_create_window("Paint", 100, 100, canvas_width, canvas_height);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);
    
    emscripten_set_interval(paint_tick, 16, NULL);
    
    return 0;
}
