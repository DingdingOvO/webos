#ifndef AUDIO_SERVER_H
#define AUDIO_SERVER_H

#include <stdint.h>

/* Maximum concurrent audio streams */
#define MAX_AUDIO_STREAMS 16

/* Stream flags */
#define AUDIO_STREAM_FLAG_DEFAULT  0x00
#define AUDIO_STREAM_FLAG_SYSTEM   0x01  /* System sounds (notifications etc) */
#define AUDIO_STREAM_FLAG_MEDIA    0x02  /* Media playback */
#define AUDIO_STREAM_FLAG_VOICE    0x04  /* Voice/chat */

/* Audio format constants (must match audio_driver.h) */
#define AUDIO_SERVER_SAMPLE_RATE   44100
#define AUDIO_SERVER_CHANNELS      2
#define AUDIO_SERVER_BUFFER_FRAMES 4096

/* Audio stream structure — one per app that wants to play audio */
typedef struct {
    int stream_id;          /* Unique stream ID */
    int owner_pid;          /* Owning process PID */
    char name[64];          /* Stream name (e.g., "MusicPlayer", "SystemSounds") */
    float volume;           /* Per-stream volume (0.0 - 1.0) */
    int paused;             /* Is stream paused? */
    int active;             /* Is stream in use? */
    int flags;              /* Stream type flags */
    float buffer[AUDIO_SERVER_BUFFER_FRAMES * AUDIO_SERVER_CHANNELS]; /* Stream buffer */
    int buffer_frames;      /* Number of frames in buffer */
    int buffer_ready;       /* Is buffer ready for mixing? */
} audio_stream_t;

/* Stream info returned to callers */
typedef struct {
    int stream_id;
    int owner_pid;
    char name[64];
    float volume;
    int paused;
    int active;
    int flags;
} audio_stream_info_t;

/* Initialize audio server */
int audio_server_init(void);

/* Shutdown audio server */
int audio_server_shutdown(void);

/* Open a new audio stream for a process */
int audio_stream_open(int pid, const char* name, int flags);

/* Close an audio stream */
int audio_stream_close(int stream_id);

/* Write PCM samples to a stream (float32 interleaved stereo) */
int audio_stream_write(int stream_id, const float* samples, int num_frames);

/* Set per-stream volume */
int audio_stream_set_volume(int stream_id, float volume);

/* Get per-stream volume */
float audio_stream_get_volume(int stream_id);

/* Pause/resume a stream */
int audio_stream_pause(int stream_id);
int audio_stream_resume(int stream_id);

/* Mix all active streams and output to driver (called by scheduler tick) */
int audio_server_mix_and_output(void);

/* Get master volume */
float audio_server_get_master_volume(void);

/* Set master volume */
int audio_server_set_master_volume(float volume);

/* Get stream info */
int audio_server_get_stream_info(int stream_id, void* info);

/* List all streams */
int audio_server_list_streams(void* info_array, int* count);

#endif /* AUDIO_SERVER_H */
