/**
 * WebOS Audio Client Library Implementation
 *
 * Client-side API that applications use to play audio. This library:
 * - Delegates core operations to the audio server (via direct function calls
 *   in the shared WASM address space)
 * - Provides convenience functions for common audio tasks:
 *   - audio_play_tone(): generate and play a sine wave at a given frequency
 *   - audio_play_noise(): generate and play white noise
 *
 * In a full OS, these would go through IPC; in the WASM single-address-space
 * model, we call the audio server functions directly via extern declarations.
 */

#include "audio_client.h"
#include "syscall.h"
#include <math.h>
#include <string.h>

/* ─── Audio Server Function Declarations ──────────────────────────── */
/* These are resolved at link time — the audio server WASM module
 * exports these symbols and they share the same address space. */

extern int audio_stream_open(int pid, const char* name, int flags);
extern int audio_stream_close(int stream_id);
extern int audio_stream_write(int stream_id, const float* samples, int num_frames);
extern int audio_stream_set_volume(int stream_id, float volume);
extern float audio_stream_get_volume(int stream_id);
extern int audio_stream_pause(int stream_id);
extern int audio_stream_resume(int stream_id);

/* ─── Simple LCG Random Number Generator ──────────────────────────── */
/* Used for white noise generation. Not cryptographically secure,
 * but perfectly adequate for audio noise. */

static uint32_t rng_state = 123456789;

static float rng_next_float(void) {
    /* LCG: x_{n+1} = x_n * 1103515245 + 12345 (glibc constants) */
    rng_state = rng_state * 1103515245u + 12345u;
    /* Map to [-1.0, 1.0] */
    return ((float)(int32_t)(rng_state & 0x7FFFFFFFu) / (float)0x7FFFFFFF) * 2.0f - 1.0f;
}

/* ─── Core API Functions ──────────────────────────────────────────── */

int audio_open(const char* name, int flags) {
    if (name == 0) {
        return -1;
    }
    int pid = syscall_getpid();
    return audio_stream_open(pid, name, flags);
}

int audio_close(int stream_id) {
    return audio_stream_close(stream_id);
}

int audio_write(int stream_id, const float* samples, int num_frames) {
    if (samples == 0 || num_frames <= 0) {
        return -1;
    }
    return audio_stream_write(stream_id, samples, num_frames);
}

int audio_set_volume(int stream_id, float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    return audio_stream_set_volume(stream_id, volume);
}

float audio_get_volume(int stream_id) {
    return audio_stream_get_volume(stream_id);
}

int audio_pause(int stream_id) {
    return audio_stream_pause(stream_id);
}

int audio_resume(int stream_id) {
    return audio_stream_resume(stream_id);
}

/* ─── Convenience: Sine Wave Tone Generator ───────────────────────── */

int audio_play_tone(int stream_id, float frequency, float duration, float volume) {
    if (frequency <= 0.0f || duration <= 0.0f || volume < 0.0f) {
        return -1;
    }

    /* Clamp volume */
    if (volume > 1.0f) volume = 1.0f;

    /* Calculate total number of frames */
    int total_frames = (int)(duration * (float)AUDIO_CLIENT_SAMPLE_RATE);
    if (total_frames <= 0) {
        return -1;
    }

    /* Working buffer — we write in chunks of AUDIO_CLIENT_BUFFER_FRAMES */
    float chunk_buffer[AUDIO_CLIENT_BUFFER_FRAMES * AUDIO_CLIENT_CHANNELS];
    int frames_written = 0;
    int phase_offset = 0; /* Track phase across chunks for continuity */

    while (frames_written < total_frames) {
        /* How many frames in this chunk? */
        int chunk_frames = total_frames - frames_written;
        if (chunk_frames > AUDIO_CLIENT_BUFFER_FRAMES) {
            chunk_frames = AUDIO_CLIENT_BUFFER_FRAMES;
        }

        /* Generate sine wave samples (interleaved stereo) */
        float two_pi = 2.0f * 3.14159265358979323846f;
        float phase_increment = two_pi * frequency / (float)AUDIO_CLIENT_SAMPLE_RATE;

        for (int i = 0; i < chunk_frames; i++) {
            float phase = (float)(frames_written + i) * phase_increment;
            float sample = sinf(phase) * volume;

            /* Interleaved stereo: [L, R, L, R, ...] */
            chunk_buffer[i * 2 + 0] = sample; /* Left  */
            chunk_buffer[i * 2 + 1] = sample; /* Right */
        }

        /* Write this chunk to the stream */
        int result = audio_stream_write(stream_id, chunk_buffer, chunk_frames);
        if (result < 0) {
            /* Write failed — return what we managed so far */
            return frames_written > 0 ? frames_written : -1;
        }

        frames_written += chunk_frames;
    }

    return frames_written;
}

/* ─── Convenience: White Noise Generator ──────────────────────────── */

int audio_play_noise(int stream_id, float duration, float volume) {
    if (duration <= 0.0f || volume < 0.0f) {
        return -1;
    }

    /* Clamp volume */
    if (volume > 1.0f) volume = 1.0f;

    /* Calculate total number of frames */
    int total_frames = (int)(duration * (float)AUDIO_CLIENT_SAMPLE_RATE);
    if (total_frames <= 0) {
        return -1;
    }

    /* Working buffer */
    float chunk_buffer[AUDIO_CLIENT_BUFFER_FRAMES * AUDIO_CLIENT_CHANNELS];
    int frames_written = 0;

    while (frames_written < total_frames) {
        int chunk_frames = total_frames - frames_written;
        if (chunk_frames > AUDIO_CLIENT_BUFFER_FRAMES) {
            chunk_frames = AUDIO_CLIENT_BUFFER_FRAMES;
        }

        /* Generate white noise samples (interleaved stereo) */
        for (int i = 0; i < chunk_frames; i++) {
            float noise_sample = rng_next_float() * volume;

            /* Interleaved stereo: [L, R, L, R, ...] — each channel
             * gets its own random sample for true stereo noise */
            chunk_buffer[i * 2 + 0] = rng_next_float() * volume; /* Left  */
            chunk_buffer[i * 2 + 1] = rng_next_float() * volume; /* Right */
        }

        /* Write this chunk to the stream */
        int result = audio_stream_write(stream_id, chunk_buffer, chunk_frames);
        if (result < 0) {
            return frames_written > 0 ? frames_written : -1;
        }

        frames_written += chunk_frames;
    }

    return frames_written;
}
