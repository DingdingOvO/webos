#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <stdint.h>

/* Audio format constants */
#define AUDIO_SAMPLE_RATE  44100
#define AUDIO_CHANNELS     2       /* Stereo */
#define AUDIO_SAMPLE_SIZE  4       /* float32 = 4 bytes */
#define AUDIO_FRAME_SIZE   (AUDIO_CHANNELS * AUDIO_SAMPLE_SIZE)  /* 8 bytes per frame */
#define AUDIO_BUFFER_FRAMES 4096   /* Buffer size in frames */

/* Audio driver initialization/shutdown */
int audio_driver_init(void);
int audio_driver_shutdown(void);

/* Write interleaved float32 PCM samples to the output device */
int audio_driver_write(const float* samples, int num_frames);

/* Get current playback position (in frames) */
uint32_t audio_driver_position(void);

/* Master volume control (0.0 - 1.0) */
int audio_driver_set_master_volume(float volume);
float audio_driver_get_master_volume(void);

/* Check if driver is initialized */
int audio_driver_is_ready(void);

#endif /* AUDIO_DRIVER_H */
