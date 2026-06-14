/**
 * WebOS Settings Application (.wex)
 *
 * System settings and preferences organized into categories:
 * - Display: wallpaper, resolution, brightness
 * - Sound: volume, notification sounds
 * - Network: WiFi, proxy
 * - Personalization: theme, accent color
 * - About: system information
 *
 * Version: 0.0.1beta
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

/* ─── Settings Categories ┼── */

enum SettingsCategory {
    CAT_DISPLAY = 0,
    CAT_SOUND,
    CAT_NETWORK,
    CAT_PERSONAL,
    CAT_ABOUT,
    CAT_COUNT,
};

struct SettingsEntry {
    const char* name;
    const char* icon;
};

static SettingsEntry categories[CAT_COUNT] = {
    {"Display",          "🖥"},
    {"Sound",            "🔊"},
    {"Network",          "📶"},
    {"Personalization",  "🎨"},
    {"About",            "ℹ️"},
};

/* ─── Settings State ┼── */

static int current_window = 0;
static int active_category = CAT_DISPLAY;

/* Display settings */
static int brightness = 100;          /* 0-100 */
static int wallpaper_index = 0;       /* 0=default gradient */
static char resolution[32] = "1920x1080";

/* Sound settings */
static int master_volume = 80;        /* 0-100 */
static int notification_volume = 70;  /* 0-100 */
static int system_sounds = 1;         /* on/off */

/* Network settings */
static int wifi_enabled = 1;
static char wifi_ssid[64] = "WebOS-Network";
static int proxy_enabled = 0;
static char proxy_addr[128] = "";

/* Personalization settings */
static int accent_color_index = 0;    /* 0=blue */
static int noise_enabled = 1;
static int noise_opacity = 3;         /* 1-5 (maps to %) */
static int blur_intensity = 2;        /* 0=low, 1=medium, 2=high */

/* Accent color options */
static const char* accent_colors[] = {
    "#3B82F6", /* Blue (default) */
    "#8B5CF6", /* Purple */
    "#EC4899", /* Pink */
    "#EF4444", /* Red */
    "#F59E0B", /* Amber */
    "#10B981", /* Emerald */
    "#06B6D4", /* Cyan */
};
static int num_accent_colors = sizeof(accent_colors) / sizeof(accent_colors[0]);

/* ─── About Info ┼── */

static const char* about_info[] = {
    "WebOS",
    "Version 0.0.1beta",
    "",
    "Architecture: WebAssembly/WebGPU",
    "Kernel: WebOS C Kernel",
    "Desktop: Frosted Glass",
    "Renderer: WebGPU",
    "Filesystem: WebFS",
    "Audio: Unified Audio Server",
    "",
    "Built with C/C++ and TypeScript",
    "Compiled to WASM via Emscripten",
    "",
    "License: MIT",
};

/* ─── Layout Constants ┼── */

#define SIDEBAR_WIDTH    200
#define CONTENT_PADDING  24
#define ROW_HEIGHT       44
#define SLIDER_HEIGHT    32
#define TOGGLE_SIZE      24

/* ─── Rendering (stub) ┼── */

static void render_display_settings(void) {
    /* In production, render via GPU:
     * - Brightness slider (0-100)
     * - Wallpaper selector (thumbnails)
     * - Resolution dropdown
     * Each row: label on left, control on right
     */
}

static void render_sound_settings(void) {
    /* In production:
     * - Master volume slider
     * - Notification volume slider
     * - System sounds toggle
     */
}

static void render_network_settings(void) {
    /* In production:
     * - WiFi toggle + SSID display
     * - Available networks list
     * - Proxy toggle + address input
     */
}

static void render_personal_settings(void) {
    /* In production:
     * - Accent color picker (7 circles)
     * - Noise toggle + opacity slider
     * - Blur intensity selector
     * - Preview panel showing effect
     */
}

static void render_about(void) {
    /* In production:
     * - WebOS logo
     * - Version info lines
     * - System specs
     * - License
     */
}

/* ─── Main Loop ┼── */

void settings_tick(void* arg) {
    /* In production:
     * 1. Poll input events
     * 2. Handle:
     *    - Category click → switch active_category
     *    - Slider drag → update value
     *    - Toggle click → flip value
     *    - Color picker → update accent_color_index
     * 3. Render:
     *    - Sidebar with category list (active highlighted blue)
     *    - Content area for active category
     *    - All using frosted glass design language
     * 4. Present via GPU driver
     */
}

int main(int argc, char** argv) {
    current_window = wm_create_window("Settings", 150, 80, 700, 520);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);

    emscripten_set_interval(settings_tick, 16, NULL);

    return 0;
}
