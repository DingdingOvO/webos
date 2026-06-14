/**
 * WebOS System Monitor Application (.wex)
 *
 * Real-time system resource monitoring dashboard:
 * - CPU usage (per-process and total)
 * - Memory usage (used/free/shared)
 * - Process list with details
 * - Audio streams overview
 * - Filesystem usage
 * - System uptime
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

    /* Process syscalls */
    #define SYS_getpid    0x0201
    #define SYS_proc_list 0x0210
    #define SYS_proc_info 0x0211
    long syscall0(long n);
    long syscall2(long n, long a1, long a2);

    /* Memory stats */
    typedef struct {
        unsigned int total_pages;
        unsigned int used_pages;
        unsigned int free_pages;
        unsigned int kernel_pages;
        unsigned int shared_pages;
    } mem_stats_t;
    void memory_stats(mem_stats_t* stats);

    #define SYS_exit 0x0200
    long syscall1(long n, long a1);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
}

/* ─── Constants ┼── */

#define MAX_PROCS         64
#define HISTORY_LENGTH    60   /* 60 seconds of history */
#define GRAPH_HEIGHT      80
#define GRAPH_WIDTH       280
#define REFRESH_INTERVAL  1000 /* ms between stats refresh */

/* ─── Process Info ┼── */

struct ProcInfo {
    unsigned int pid;
    char name[32];
    unsigned int state;       /* 0=ready, 1=running, 2=blocked, 3=zombie */
    unsigned int priority;
    unsigned int memory_kb;
    float cpu_percent;
};

/* ─── Monitor State ┼── */

static int current_window = 0;
static int active_tab = 0;  /* 0=overview, 1=processes, 2=memory, 3=filesystem */

/* System stats */
static mem_stats_t mem_stats = {0};
static float cpu_usage = 0.0f;
static unsigned int uptime_seconds = 0;

/* History graphs */
static float cpu_history[HISTORY_LENGTH] = {0};
static float mem_history[HISTORY_LENGTH] = {0};
static int history_index = 0;

/* Process list */
static ProcInfo proc_list[MAX_PROCS];
static int proc_count = 0;
static int selected_proc = -1;
static int proc_scroll = 0;
static char proc_sort_field = 'c'; /* 'c'=cpu, 'm'=memory, 'p'=pid */

/* Filesystem info */
static unsigned int fs_total_blocks = 10240;
static unsigned int fs_used_blocks = 2048;
static unsigned int fs_inode_count = 500;
static unsigned int fs_inode_used = 42;

/* Audio info */
static int audio_streams_active = 0;
static int audio_streams_total = 16;
static float audio_master_volume = 0.8f;

/* ─── Data Collection ┼── */

static void collect_stats(void) {
    /* Memory stats */
    memory_stats(&mem_stats);

    /* CPU usage simulation (in production: actual CPU time measurement) */
    /* Simple model: based on number of running processes */
    cpu_usage = 0.1f + 0.15f * proc_count;
    if (cpu_usage > 1.0f) cpu_usage = 1.0f;

    /* Update history */
    cpu_history[history_index % HISTORY_LENGTH] = cpu_usage;
    float mem_pct = 0;
    if (mem_stats.total_pages > 0) {
        mem_pct = (float)mem_stats.used_pages / mem_stats.total_pages;
    }
    mem_history[history_index % HISTORY_LENGTH] = mem_pct;
    history_index++;

    /* Uptime */
    uptime_seconds++;

    /* Process list (in production: syscall to kernel) */
    proc_count = 0;

    /* Kernel process */
    proc_list[proc_count].pid = 0;
    strncpy(proc_list[proc_count].name, "kernel", sizeof(proc_list[0].name));
    proc_list[proc_count].state = 1;
    proc_list[proc_count].priority = 3;
    proc_list[proc_count].memory_kb = 256;
    proc_list[proc_count].cpu_percent = 2.0f;
    proc_count++;

    /* System services */
    const char* service_names[] = {
        "wm.Wdll", "fs_service.Wdll", "audio_server.Wdll",
        "net_service.Wdll", "appstore.Wdll"
    };
    for (int i = 0; i < 5 && proc_count < MAX_PROCS; i++) {
        proc_list[proc_count].pid = i + 2;
        strncpy(proc_list[proc_count].name, service_names[i], sizeof(proc_list[0].name));
        proc_list[proc_count].state = 1;
        proc_list[proc_count].priority = 2;
        proc_list[proc_count].memory_kb = 64 + i * 16;
        proc_list[proc_count].cpu_percent = 0.5f + (float)(i % 3) * 0.3f;
        proc_count++;
    }

    /* Running apps */
    const char* app_names[] = {"sys_monitor.wex"};
    for (int i = 0; i < 1 && proc_count < MAX_PROCS; i++) {
        proc_list[proc_count].pid = proc_count + 1;
        strncpy(proc_list[proc_count].name, app_names[i], sizeof(proc_list[0].name));
        proc_list[proc_count].state = 1;
        proc_list[proc_count].priority = 1;
        proc_list[proc_count].memory_kb = 128;
        proc_list[proc_count].cpu_percent = 1.5f;
        proc_count++;
    }

    audio_streams_active = 0; /* In production: query audio server */
}

/* ─── Format Helpers ┼── */

static void format_bytes(unsigned int kb, char* out, int out_len) {
    if (kb < 1024) {
        snprintf(out, out_len, "%u KB", kb);
    } else if (kb < 1024 * 1024) {
        snprintf(out, out_len, "%.1f MB", kb / 1024.0f);
    } else {
        snprintf(out, out_len, "%.1f GB", kb / (1024.0f * 1024.0f));
    }
}

static void format_uptime(unsigned int sec, char* out, int out_len) {
    unsigned int h = sec / 3600;
    unsigned int m = (sec % 3600) / 60;
    unsigned int s = sec % 60;
    if (h > 0) {
        snprintf(out, out_len, "%uh %um %us", h, m, s);
    } else {
        snprintf(out, out_len, "%um %us", m, s);
    }
}

static const char* state_name(unsigned int state) {
    switch (state) {
        case 0: return "Ready";
        case 1: return "Running";
        case 2: return "Blocked";
        case 3: return "Zombie";
        default: return "Unknown";
    }
}

/* ─── Main Loop ┼── */

static unsigned int last_collect = 0;

void monitor_tick(void* arg) {
    /* Collect stats every ~1 second */
    unsigned int now = 0; /* In production: syscall for time */
    if (now - last_collect >= REFRESH_INTERVAL / 16) {
        collect_stats();
        last_collect = now;
    }

    /* In production:
     * 1. Poll input events
     * 2. Handle:
     *    - Tab click → switch active_tab
     *    - Process list row click → select
     *    - Kill process button → process_kill(pid)
     *    - Sort column header click → change sort
     * 3. Render based on active_tab:
     *
     *    Overview tab:
     *    - CPU usage gauge + graph (last 60s)
     *    - Memory usage gauge + graph
     *    - Audio streams count
     *    - Uptime
     *    - Quick stats cards
     *
     *    Processes tab:
     *    - Sortable table: PID, Name, State, Priority, Memory, CPU%
     *    - Kill Process button
     *    - Refresh button
     *
     *    Memory tab:
     *    - Detailed memory breakdown
     *    - Page allocation bar chart
     *    - Shared memory regions
     *
     *    Filesystem tab:
     *    - Storage usage bar (used/total)
     *    - Inode usage bar
     *    - Block size info
     *    - Mount point details
     *
     * 4. All using frosted glass design language with:
     *    - Dark background (#0F172A)
     *    - Elevated surface cards (#1E293B/0.72)
     *    - Blue accent (#3B82F6) for graphs and highlights
     *    - Noise texture on background surfaces
     * 5. Present via GPU driver
     */
}

int main(int argc, char** argv) {
    current_window = wm_create_window("System Monitor", 160, 70, 780, 560);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);

    /* Initial data collection */
    collect_stats();

    emscripten_set_interval(monitor_tick, 16, NULL);

    return 0;
}
