/**
 * WebOS Audio Server Service (.Wdll)
 *
 * Central audio mixer for WebOS — analogous to PulseAudio/PipeWire in Linux.
 * All apps send their audio streams to this server, which mixes everything
 * together and plays the combined output through a single audio device.
 *
 * Architecture:
 *   App 1 (stream) ──┐
 *   App 2 (stream) ──┼──▶ Audio Server (mixer) ──▶ Audio Driver ──▶ Web Audio API
 *   App 3 (stream) ──┘
 *
 * Key design:
 * - Each app opens an audio_stream_t to submit PCM samples
 * - The scheduler calls audio_server_mix_and_output() periodically
 * - Mixing: sum all streams * per-stream volume, then * master volume
 * - Soft clipping (tanh) prevents distortion from over-amplification
 * - The mixed result is written to the audio driver for playback
 */

#include "audio_server.h"
#include <emscripten.h>
#include <string.h>
#include <math.h>

/* Reference the audio driver (loaded as a separate WASM module) */
extern int audio_driver_init(void);
extern int audio_driver_shutdown(void);
extern int audio_driver_write(const float* samples, int num_frames);
extern int audio_driver_set_master_volume(float volume);
extern float audio_driver_get_master_volume(void);
extern int audio_driver_is_ready(void);

/* ─── Internal State ──────────────────────────────────────────────── */

static audio_stream_t streams[MAX_AUDIO_STREAMS];
static int audio_server_initialized = 0;
static float master_volume = 0.8f;
static int next_stream_id = 1;

/* Mixing buffer — large enough for one full buffer of mixed audio */
static float mix_buffer[AUDIO_SERVER_BUFFER_FRAMES * AUDIO_SERVER_CHANNELS];

/* ─── Helper Functions ────────────────────────────────────────────── */

/**
 * Find a stream slot by stream_id.
 * Returns pointer to the stream, or NULL if not found / inactive.
 */
static audio_stream_t* find_stream(int stream_id) {
    if (stream_id <= 0) return 0;
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (streams[i].active && streams[i].stream_id == stream_id) {
            return &streams[i];
        }
    }
    return 0;
}

/**
 * Find a free stream slot.
 * Returns index into streams[], or -1 if full.
 */
static int find_free_slot(void) {
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (!streams[i].active) {
            return i;
        }
    }
    return -1;
}

/**
 * Clamp a float value to [lo, hi].
 */
static float clampf(float value, float lo, float hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

/**
 * Fast tanh approximation using rational function.
 * Accurate to ~0.01 over the range [-8, 8].
 */
static float fast_tanh(float x) {
    /* For very large values, just return sign */
    if (x > 8.0f) return 1.0f;
    if (x < -8.0f) return -1.0f;

    /* Rational approximation: tanh(x) ≈ x * (27 + x²) / (27 + 9*x²) */
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/* ─── Audio Server Lifecycle ──────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
int audio_server_init(void) {
    if (audio_server_initialized) {
        return 0; /* Already initialized — idempotent */
    }

    /* Initialize the audio driver (connects to Web Audio API) */
    int driver_result = audio_driver_init();
    if (driver_result < 0) {
        return -1;
    }

    /* Clear all stream slots */
    memset(streams, 0, sizeof(streams));
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        streams[i].active = 0;
        streams[i].stream_id = 0;
        streams[i].buffer_ready = 0;
        streams[i].buffer_frames = 0;
    }

    /* Set default master volume */
    master_volume = 0.8f;
    next_stream_id = 1;

    /* Propagate master volume to driver */
    audio_driver_set_master_volume(master_volume);

    audio_server_initialized = 1;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int audio_server_shutdown(void) {
    if (!audio_server_initialized) {
        return -1;
    }

    /* Close all active streams */
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (streams[i].active) {
            streams[i].active = 0;
            streams[i].buffer_ready = 0;
        }
    }

    /* Shut down the audio driver */
    audio_driver_shutdown();

    audio_server_initialized = 0;
    master_volume = 0.0f;
    return 0;
}

/* ─── Stream Management ───────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
int audio_stream_open(int pid, const char* name, int flags) {
    if (!audio_server_initialized) {
        return -1;
    }
    if (name == 0) {
        return -1;
    }

    int slot = find_free_slot();
    if (slot < 0) {
        return -1; /* No free stream slots */
    }

    audio_stream_t* s = &streams[slot];
    s->stream_id = next_stream_id++;
    s->owner_pid = pid;
    s->volume = 1.0f;
    s->paused = 0;
    s->active = 1;
    s->flags = flags;
    s->buffer_frames = 0;
    s->buffer_ready = 0;

    /* Copy stream name (safe, bounded) */
    int len = 0;
    while (name[len] && len < 63) {
        s->name[len] = name[len];
        len++;
    }
    s->name[len] = '\0';

    /* Zero the buffer */
    memset(s->buffer, 0, sizeof(s->buffer));

    return s->stream_id;
}

EMSCRIPTEN_KEEPALIVE
int audio_stream_close(int stream_id) {
    if (!audio_server_initialized) {
        return -1;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return -1; /* Stream not found */
    }

    s->active = 0;
    s->buffer_ready = 0;
    s->buffer_frames = 0;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int audio_stream_write(int stream_id, const float* samples, int num_frames) {
    if (!audio_server_initialized) {
        return -1;
    }
    if (samples == 0) {
        return -1;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return -1; /* Stream not found */
    }
    if (s->paused) {
        return -1; /* Stream is paused — cannot write */
    }

    /* Clamp to buffer capacity */
    int frames_to_write = num_frames;
    if (frames_to_write > AUDIO_SERVER_BUFFER_FRAMES) {
        frames_to_write = AUDIO_SERVER_BUFFER_FRAMES;
    }
    if (frames_to_write <= 0) {
        return 0;
    }

    /* Copy samples into the stream buffer (interleaved stereo float32) */
    int total_samples = frames_to_write * AUDIO_SERVER_CHANNELS;
    memcpy(s->buffer, samples, total_samples * sizeof(float));

    s->buffer_frames = frames_to_write;
    s->buffer_ready = 1;

    return frames_to_write;
}

/* ─── Per-Stream Volume Control ───────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
int audio_stream_set_volume(int stream_id, float volume) {
    if (!audio_server_initialized) {
        return -1;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return -1;
    }

    s->volume = clampf(volume, 0.0f, 1.0f);
    return 0;
}

EMSCRIPTEN_KEEPALIVE
float audio_stream_get_volume(int stream_id) {
    if (!audio_server_initialized) {
        return 0.0f;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return 0.0f;
    }

    return s->volume;
}

/* ─── Pause / Resume ──────────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
int audio_stream_pause(int stream_id) {
    if (!audio_server_initialized) {
        return -1;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return -1;
    }

    s->paused = 1;
    /* Clear buffer so stale audio isn't played on resume */
    s->buffer_ready = 0;
    s->buffer_frames = 0;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int audio_stream_resume(int stream_id) {
    if (!audio_server_initialized) {
        return -1;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return -1;
    }

    s->paused = 0;
    return 0;
}

/* ─── THE KEY FUNCTION: Mix and Output ────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
int audio_server_mix_and_output(void) {
    if (!audio_server_initialized) {
        return -1;
    }

    /* Zero the mixing buffer */
    memset(mix_buffer, 0, sizeof(mix_buffer));

    /* Count how many streams contribute to the mix */
    int active_count = 0;

    /* Mix all active, non-paused streams that have data ready */
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        audio_stream_t* s = &streams[i];

        if (!s->active || s->paused || !s->buffer_ready) {
            continue;
        }

        active_count++;

        /* Determine how many frames this stream contributes */
        int stream_frames = s->buffer_frames;
        if (stream_frames > AUDIO_SERVER_BUFFER_FRAMES) {
            stream_frames = AUDIO_SERVER_BUFFER_FRAMES;
        }

        /* Add this stream's samples to the mix buffer,
         * multiplied by the stream's per-stream volume.
         *
         * Formula: mixed_sample += stream_sample * stream_volume
         */
        int total_samples = stream_frames * AUDIO_SERVER_CHANNELS;
        for (int j = 0; j < total_samples; j++) {
            mix_buffer[j] += s->buffer[j] * s->volume;
        }

        /* Stream buffer consumed — mark as not ready */
        s->buffer_ready = 0;
    }

    /* If no streams contributed, output silence */
    int output_frames = AUDIO_SERVER_BUFFER_FRAMES;

    if (active_count > 0) {
        /* Apply master volume to the entire mix:
         * final_sample = mixed_sample * master_volume
         */
        int total_samples = output_frames * AUDIO_SERVER_CHANNELS;
        for (int j = 0; j < total_samples; j++) {
            mix_buffer[j] *= master_volume;

            /* Apply soft clipping using tanh to prevent harsh distortion.
             * tanh naturally maps (-inf, +inf) to (-1, 1), providing
             * smooth saturation instead of hard clipping.
             */
            mix_buffer[j] = fast_tanh(mix_buffer[j]);
        }
    }
    /* else: mix_buffer is already zeroed — silence */

    /* Write the mixed output to the audio driver */
    int result = audio_driver_write(mix_buffer, output_frames);
    if (result < 0) {
        return -1;
    }

    return 0;
}

/* ─── Master Volume Control ───────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
float audio_server_get_master_volume(void) {
    if (!audio_server_initialized) {
        return 0.0f;
    }
    return master_volume;
}

EMSCRIPTEN_KEEPALIVE
int audio_server_set_master_volume(float volume) {
    if (!audio_server_initialized) {
        return -1;
    }

    master_volume = clampf(volume, 0.0f, 1.0f);

    /* Propagate to the audio driver so it can update its gain node */
    audio_driver_set_master_volume(master_volume);

    return 0;
}

/* ─── Stream Information ──────────────────────────────────────────── */

EMSCRIPTEN_KEEPALIVE
int audio_server_get_stream_info(int stream_id, void* info) {
    if (!audio_server_initialized || info == 0) {
        return -1;
    }

    audio_stream_t* s = find_stream(stream_id);
    if (s == 0) {
        return -1;
    }

    audio_stream_info_t* out = (audio_stream_info_t*)info;
    out->stream_id = s->stream_id;
    out->owner_pid = s->owner_pid;
    out->volume = s->volume;
    out->paused = s->paused;
    out->active = s->active;
    out->flags = s->flags;

    /* Copy name safely */
    int len = 0;
    while (s->name[len] && len < 63) {
        out->name[len] = s->name[len];
        len++;
    }
    out->name[len] = '\0';

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int audio_server_list_streams(void* info_array, int* count) {
    if (!audio_server_initialized || info_array == 0 || count == 0) {
        return -1;
    }

    audio_stream_info_t* infos = (audio_stream_info_t*)info_array;
    int written = 0;

    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (!streams[i].active) continue;

        audio_stream_t* s = &streams[i];
        audio_stream_info_t* out = &infos[written];

        out->stream_id = s->stream_id;
        out->owner_pid = s->owner_pid;
        out->volume = s->volume;
        out->paused = s->paused;
        out->active = s->active;
        out->flags = s->flags;

        /* Copy name safely */
        int len = 0;
        while (s->name[len] && len < 63) {
            out->name[len] = s->name[len];
            len++;
        }
        out->name[len] = '\0';

        written++;
    }

    *count = written;
    return 0;
}
