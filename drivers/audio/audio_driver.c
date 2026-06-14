/**
 * WebOS Audio Driver (.Wdll)
 *
 * Low-level interface to the Web Audio API. This driver provides:
 * - A single output device (AudioContext)
 * - Master volume control
 * - Sample rate: 44100 Hz
 * - Format: Float32 interleaved PCM samples (stereo)
 *
 * Uses Emscripten to call JS host functions that interface with
 * the Web Audio API in the browser.
 */

#include "audio_driver.h"
#include <emscripten.h>
#include <math.h>

/* JS host function declarations — implemented in host/src/runtime_bridge.ts */
extern int js_audio_init(int sample_rate, int channels);
extern int js_audio_write(const float* samples, int num_frames);
extern int js_audio_set_volume(float volume);
extern float js_audio_get_volume(void);

/* Internal driver state */
static int audio_driver_initialized = 0;
static float audio_driver_master_volume = 1.0f;
static uint32_t audio_driver_playback_position = 0;

EMSCRIPTEN_KEEPALIVE
int audio_driver_init(void) {
    if (audio_driver_initialized) {
        /* Already initialized — idempotent */
        return 0;
    }

    int result = js_audio_init(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS);
    if (result < 0) {
        return -1;
    }

    audio_driver_initialized = 1;
    audio_driver_master_volume = 1.0f;
    audio_driver_playback_position = 0;

    /* Propagate initial volume to JS side */
    js_audio_set_volume(audio_driver_master_volume);

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int audio_driver_shutdown(void) {
    if (!audio_driver_initialized) {
        return -1;
    }

    audio_driver_initialized = 0;
    audio_driver_master_volume = 0.0f;
    audio_driver_playback_position = 0;

    /* Mute on shutdown */
    js_audio_set_volume(0.0f);

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int audio_driver_write(const float* samples, int num_frames) {
    if (!audio_driver_initialized) {
        return -1;
    }
    if (samples == 0) {
        return -1;
    }
    if (num_frames <= 0 || num_frames > AUDIO_BUFFER_FRAMES) {
        return -1;
    }

    int result = js_audio_write(samples, num_frames);
    if (result < 0) {
        return -1;
    }

    /* Advance playback position */
    audio_driver_playback_position += (uint32_t)num_frames;

    return num_frames;
}

EMSCRIPTEN_KEEPALIVE
uint32_t audio_driver_position(void) {
    if (!audio_driver_initialized) {
        return 0;
    }
    return audio_driver_playback_position;
}

EMSCRIPTEN_KEEPALIVE
int audio_driver_set_master_volume(float volume) {
    if (!audio_driver_initialized) {
        return -1;
    }

    /* Clamp to [0.0, 1.0] */
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    audio_driver_master_volume = volume;

    /* Propagate to JS AudioContext gain node */
    js_audio_set_volume(volume);

    return 0;
}

EMSCRIPTEN_KEEPALIVE
float audio_driver_get_master_volume(void) {
    if (!audio_driver_initialized) {
        return 0.0f;
    }
    return audio_driver_master_volume;
}

EMSCRIPTEN_KEEPALIVE
int audio_driver_is_ready(void) {
    return audio_driver_initialized;
}
