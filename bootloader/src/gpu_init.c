/**
 * WebOS GPU Initialization Implementation
 * 
 * Initializes the GPU by loading the GPU driver module
 * and setting up the display framebuffer.
 */

#include "gpu_init.h"

/* Import syscall from host bridge */
__attribute__((import_module("webos"), import_name("syscall_invoke")))
extern int32_t syscall_invoke(uint32_t num, int32_t a1, int32_t a2,
                               int32_t a3, int32_t a4, int32_t a5);

/* Syscall numbers */
#define SYS_WM_CREATE  50
#define SYS_WM_RENDER  54

/* Boot splash window dimensions */
#define BOOT_SPLASH_WIDTH  640
#define BOOT_SPLASH_HEIGHT 480

/* Framebuffer state */
static gpu_pixel_t* framebuffer = (gpu_pixel_t*)0;
static int32_t fb_width = 0;
static int32_t fb_height = 0;

int32_t gpu_init(void) {
    /* Set default dimensions */
    fb_width = BOOT_SPLASH_WIDTH;
    fb_height = BOOT_SPLASH_HEIGHT;

    /* In the full implementation, we would load gpu_driver.Wdll
     * and call its initialization function. For now, we create
     * a basic display window via the window manager syscall. */

    /* Create a boot splash window */
    int32_t result = syscall_invoke(SYS_WM_CREATE,
                                     (int32_t)"WebOS Boot", 10,
                                     0 | (0 << 16),
                                     BOOT_SPLASH_WIDTH | (BOOT_SPLASH_HEIGHT << 16),
                                     0x20); /* WM_FLAG_FULLSCREEN */

    if (result <= 0) {
        return -1;
    }

    /* Get framebuffer pointer */
    framebuffer = (gpu_pixel_t*)syscall_invoke(SYS_WM_RENDER,
                                                 result, 0, 0, 0, 0);
    return 0;
}

int32_t gpu_set_mode(int32_t mode) {
    /* Mode switching will be implemented with GPU driver */
    (void)mode;
    return 0;
}

gpu_pixel_t* gpu_get_framebuffer(void) {
    return framebuffer;
}

void gpu_get_dimensions(int32_t* width, int32_t* height) {
    if (width) *width = fb_width;
    if (height) *height = fb_height;
}

void gpu_clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!framebuffer) return;

    int32_t total = fb_width * fb_height;
    for (int32_t i = 0; i < total; i++) {
        framebuffer[i].r = r;
        framebuffer[i].g = g;
        framebuffer[i].b = b;
        framebuffer[i].a = a;
    }
}

void gpu_present(void) {
    /* Present framebuffer via WM syscall */
    if (!framebuffer) return;
    syscall_invoke(SYS_WM_RENDER, 1, 1, 0, 0, 0);
}
