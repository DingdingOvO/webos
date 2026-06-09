#ifndef GPU_INIT_H
#define GPU_INIT_H

/**
 * WebOS GPU Initialization Interface
 * 
 * Initializes the GPU driver and sets up the display
 * framebuffer for the boot splash and kernel use.
 */

#include <stdint.h>

/* Display modes */
#define GPU_MODE_640x480    0
#define GPU_MODE_800x600    1
#define GPU_MODE_1024x768   2
#define GPU_MODE_1920x1080  3
#define GPU_MODE_NATIVE     4

/* Pixel format: RGBA8888 */
typedef struct {
    uint8_t r, g, b, a;
} gpu_pixel_t;

/**
 * Initialize the GPU subsystem.
 * Sets up the display at the given resolution.
 * 
 * @return  0 on success, negative on error
 */
int32_t gpu_init(void);

/**
 * Set the display mode.
 * 
 * @param mode  Display mode constant
 * @return      0 on success, negative on error
 */
int32_t gpu_set_mode(int32_t mode);

/**
 * Get the current framebuffer pointer.
 * 
 * @return  Pointer to the RGBA framebuffer
 */
gpu_pixel_t* gpu_get_framebuffer(void);

/**
 * Get current display dimensions.
 * 
 * @param width   Output: width in pixels
 * @param height  Output: height in pixels
 */
void gpu_get_dimensions(int32_t* width, int32_t* height);

/**
 * Clear the framebuffer with a solid color.
 * 
 * @param r, g, b, a  Color components
 */
void gpu_clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * Present (flip) the framebuffer to the display.
 */
void gpu_present(void);

#endif /* GPU_INIT_H */
