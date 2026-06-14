/**
 * WebOS Music Player Application (.wex)
 *
 * An audio player with playlist management, visualization,
 * and playback controls. Uses the audio server service for
 * audio output and mixing.
 *
 * Features:
 * - Play/Pause/Stop/Skip
 * - Playlist management
 * - Volume control
 * - Progress bar with seeking
 * - Audio visualization (waveform/spectrum)
 * - Album art display
 *
 * Version: 0.0.1beta
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <emscripten.h>

extern "C" {
    int wm_create_window(const char* title, int x, int y, int width, int height);
    void wm_show_window(int window_id);
    void wm_destroy_window(int window_id);

    /* Audio client library */
    int audio_open(int sample_rate, int channels);
    int audio_close(int stream_id);
    int audio_write(int stream_id, const void* data, int frames);
    int audio_set_volume(int stream_id, float volume);
    int audio_pause(int stream_id);
    int audio_resume(int stream_id);

    /* Filesystem service */
    int fs_open(const char* path, int flags);
    int fs_read(int fd, void* buf, int count);
    int fs_close(int fd);
    int fs_listdir(const char* path, void* entries, int max_entries);

    #define SYS_exit 0x0200
    long syscall1(long n, long a1);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
}

/* ─── Constants ┼── */

#define MAX_PLAYLIST     64
#define VISUALIZER_BARS  32
#define SAMPLE_RATE      44100

/* ─── Playlist Entry ┼── */

struct Track {
    char title[128];
    char artist[128];
    char path[256];
    int duration_ms;     /* 0 = unknown */
    int is_playing;
};

/* ─── Player State ┼── */

static int current_window = 0;
static int audio_stream = -1;

static Track playlist[MAX_PLAYLIST];
static int playlist_count = 0;
static int current_track = -1;

enum PlayState {
    STATE_STOPPED = 0,
    STATE_PLAYING = 1,
    STATE_PAUSED = 2,
};
static int play_state = STATE_STOPPED;
static float volume = 0.8f;

/* Progress tracking */
static int position_ms = 0;
static int duration_ms = 0;

/* Visualizer data */
static float visualizer_data[VISUALIZER_BARS];

/* Repeat/shuffle */
static int repeat_mode = 0;    /* 0=off, 1=all, 2=one */
static int shuffle_mode = 0;

/* ─── Playlist Management ┼── */

static void add_track(const char* title, const char* artist, const char* path, int duration) {
    if (playlist_count >= MAX_PLAYLIST) return;
    Track* t = &playlist[playlist_count];
    strncpy(t->title, title, sizeof(t->title) - 1);
    strncpy(t->artist, artist, sizeof(t->artist) - 1);
    strncpy(t->path, path, sizeof(t->path) - 1);
    t->duration_ms = duration;
    t->is_playing = 0;
    playlist_count++;
}

static void remove_track(int index) {
    if (index < 0 || index >= playlist_count) return;
    for (int i = index; i < playlist_count - 1; i++) {
        playlist[i] = playlist[i + 1];
    }
    playlist_count--;
    if (current_track >= playlist_count) current_track = playlist_count - 1;
}

/* ─── Playback Control ┼── */

static void play_track(int index) {
    if (index < 0 || index >= playlist_count) return;

    /* Stop current */
    if (audio_stream >= 0) {
        audio_pause(audio_stream);
    }

    /* Mark all as not playing */
    for (int i = 0; i < playlist_count; i++) {
        playlist[i].is_playing = 0;
    }

    current_track = index;
    playlist[index].is_playing = 1;
    play_state = STATE_PLAYING;
    position_ms = 0;
    duration_ms = playlist[index].duration_ms;

    /* In production: open audio stream and start decoding the file */
    /* For now, generate a test tone */
    if (audio_stream < 0) {
        audio_stream = audio_open(SAMPLE_RATE, 2);
    }
    if (audio_stream >= 0) {
        audio_set_volume(audio_stream, volume);
        audio_resume(audio_stream);
    }
}

static void pause_playback(void) {
    if (play_state == STATE_PLAYING && audio_stream >= 0) {
        audio_pause(audio_stream);
        play_state = STATE_PAUSED;
    }
}

static void resume_playback(void) {
    if (play_state == STATE_PAUSED && audio_stream >= 0) {
        audio_resume(audio_stream);
        play_state = STATE_PLAYING;
    }
}

static void stop_playback(void) {
    if (audio_stream >= 0) {
        audio_pause(audio_stream);
    }
    play_state = STATE_STOPPED;
    position_ms = 0;
    if (current_track >= 0 && current_track < playlist_count) {
        playlist[current_track].is_playing = 0;
    }
    current_track = -1;
}

static void next_track(void) {
    if (playlist_count == 0) return;
    int next = (current_track + 1) % playlist_count;
    play_track(next);
}

static void prev_track(void) {
    if (playlist_count == 0) return;
    int prev = (current_track - 1 + playlist_count) % playlist_count;
    play_track(prev);
}

static void set_volume(float v) {
    volume = v;
    if (volume < 0) volume = 0;
    if (volume > 1) volume = 1;
    if (audio_stream >= 0) {
        audio_set_volume(audio_stream, volume);
    }
}

/* ─── Visualizer ┼── */

static void update_visualizer(void) {
    /* In production: FFT analysis of audio output */
    /* For now: generate a simple animated visualization */
    static float phase = 0;
    phase += 0.05f;

    for (int i = 0; i < VISUALIZER_BARS; i++) {
        float freq = (float)(i + 1) / VISUALIZER_BARS;
        if (play_state == STATE_PLAYING) {
            visualizer_data[i] = 0.3f + 0.5f * sinf(phase * (2.0f + freq * 4.0f) + i * 0.3f);
            visualizer_data[i] = fabsf(visualizer_data[i]);
        } else {
            /* Decay to zero when not playing */
            visualizer_data[i] *= 0.9f;
        }
    }
}

/* ─── Format Helpers ┼── */

static void format_time(int ms, char* out, int out_len) {
    int total_sec = ms / 1000;
    int min = total_sec / 60;
    int sec = total_sec % 60;
    snprintf(out, out_len, "%d:%02d", min, sec);
}

/* ─── Main Loop ┼── */

void music_tick(void* arg) {
    /* Update position */
    if (play_state == STATE_PLAYING) {
        position_ms += 16; /* ~60fps tick */
        if (duration_ms > 0 && position_ms >= duration_ms) {
            /* Track ended */
            if (repeat_mode == 2) {
                play_track(current_track);  /* Repeat one */
            } else {
                next_track();
            }
        }
    }

    /* Update visualizer */
    update_visualizer();

    /* In production:
     * 1. Poll input events
     * 2. Handle:
     *    - Play/Pause button click
     *    - Next/Prev button click
     *    - Volume slider drag
     *    - Progress bar click (seek)
     *    - Playlist entry click → play_track()
     *    - Repeat/Shuffle toggle
     * 3. Render:
     *    - Album art area (or placeholder gradient)
     *    - Track title + artist
     *    - Progress bar with time labels
     *    - Playback controls (⏮ ▶ ⏭)
     *    - Volume slider with speaker icon
     *    - Visualizer bars (blue accent gradient)
     *    - Playlist panel on the right
     *    - All using frosted glass design language
     * 4. Present via GPU driver
     */
}

int main(int argc, char** argv) {
    current_window = wm_create_window("Music Player", 130, 100, 640, 480);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);

    /* Add some demo tracks */
    add_track("WebOS Theme",       "System",    "/system/audio/theme.wav",    180000);
    add_track("Startup Sound",     "System",    "/system/audio/startup.wav",   3000);
    add_track("Notification",      "System",    "/system/audio/notify.wav",    1500);
    add_track("Ambient Blue",      "WebOS",     "/music/ambient_blue.wav",   240000);
    add_track("Digital Dreams",    "WebOS",     "/music/digital_dreams.wav", 195000);

    emscripten_set_interval(music_tick, 16, NULL);

    return 0;
}
