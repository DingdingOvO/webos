#ifndef AUDIO_CLIENT_H
#define AUDIO_CLIENT_H

#include <stdint.h>

/**
 * WebOS Audio Client Interface
 *
 * Client-side API for playing audio through the WebOS audio server.
 * Applications include this header to open streams, write PCM samples,
 * and use convenience functions for tone/noise generation.
 *
 * Usage:
 *   int stream = audio_open("MyApp", AUDIO_STREAM_FLAG_MEDIA);
 *   audio_set_volume(stream, 0.7f);
 *   audio_play_tone(stream, 440.0f, 1.0f, 0.5f);  // Play A440 for 1 second
 *   audio_close(stream);
 */

/* Stream type flags (match audio_server.h) */
#define AUDIO_CLIENT_FLAG_DEFAULT  0x00
#define AUDIO_CLIENT_FLAG_SYSTEM   0x01
#define AUDIO_CLIENT_FLAG_MEDIA    0x02
#define AUDIO_CLIENT_FLAG_VOICE    0x04

/* Audio format constants */
#define AUDIO_CLIENT_SAMPLE_RATE   44100
#define AUDIO_CLIENT_CHANNELS      2
#define AUDIO_CLIENT_BUFFER_FRAMES 4096

/* Open an audio stream (delegates to audio_stream_open) */
int audio_open(const char* name, int flags);

/* Close audio stream */
int audio_close(int stream_id);

/* Write PCM samples (float32 interleaved stereo) */
int audio_write(int stream_id, const float* samples, int num_frames);

/* Set volume for this stream (0.0 - 1.0) */
int audio_set_volume(int stream_id, float volume);

/* Get volume */
float audio_get_volume(int stream_id);

/* Pause stream */
int audio_pause(int stream_id);

/* Resume stream */
int audio_resume(int stream_id);

/**
 * Convenience: generate a sine wave tone.
 *
 * @param stream_id  Stream to write to
 * @param frequency  Tone frequency in Hz (e.g., 440.0 for A4)
 * @param duration   Duration in seconds
 * @param volume     Volume for this tone (0.0 - 1.0)
 * @return           Number of frames written, or negative on error
 */
int audio_play_tone(int stream_id, float frequency, float duration, float volume);

/**
 * Convenience: generate white noise.
 *
 * @param stream_id  Stream to write to
 * @param duration   Duration in seconds
 * @param volume     Volume for this noise (0.0 - 1.0)
 * @return           Number of frames written, or negative on error
 */
int audio_play_noise(int stream_id, float duration, float volume);

#endif /* AUDIO_CLIENT_H */
