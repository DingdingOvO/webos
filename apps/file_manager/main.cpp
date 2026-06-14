/**
 * WebOS File Manager Application (.wex)
 *
 * A graphical file manager with sidebar navigation, file grid/list view,
 * context menus, and file operations (copy, move, delete, rename).
 * Provides a visual interface to the WebFS filesystem.
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

    /* Filesystem service */
    int fs_open(const char* path, int flags);
    int fs_read(int fd, void* buf, int count);
    int fs_write(int fd, const void* buf, int count);
    int fs_close(int fd);
    int fs_mkdir(const char* path);
    int fs_listdir(const char* path, void* entries, int max_entries);
    int fs_stat(const char* path, void* stat_buf);
    int fs_unlink(const char* path);

    #define SYS_exit 0x0200
    long syscall1(long n, long a1);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
}

/* ─── Layout Constants ─── */

#define SIDEBAR_WIDTH     180
#define TOOLBAR_HEIGHT    40
#define STATUSBAR_HEIGHT  28
#define ICON_SIZE         48
#define ICON_PADDING      12
#define GRID_COLS         4

/* ─── File Entry ─── */

enum FileType {
    FILE_REGULAR = 0,
    FILE_DIRECTORY = 1,
    FILE_SYMLINK = 2,
};

struct FileEntry {
    char name[128];
    FileType type;
    unsigned int size;
    unsigned int modified;
};

/* ─── File Manager State ─── */

static int current_window = 0;
static char current_path[512] = "/";
static char clipboard_path[512] = "";
static int clipboard_cut = 0;

/* File listing */
#define MAX_ENTRIES 128
static FileEntry entries[MAX_ENTRIES];
static int entry_count = 0;
static int selected_entry = -1;
static int scroll_y = 0;

/* View mode: 0 = grid, 1 = list */
static int view_mode = 0;

/* Sidebar favorites */
struct Favorite {
    const char* name;
    const char* path;
    const char* icon;
};

static Favorite favorites[] = {
    {"Home",        "/home/user",    "🏠"},
    {"Desktop",     "/home/user/Desktop", "🖥"},
    {"Documents",   "/home/user/Documents", "📄"},
    {"Downloads",   "/home/user/Downloads", "⬇"},
    {"Pictures",    "/home/user/Pictures", "🖼"},
    {"Music",       "/home/user/Music",    "🎵"},
    {"Root",        "/",             "💾"},
};
static int num_favorites = sizeof(favorites) / sizeof(favorites[0]);

/* ─── Navigation ┼── */

static void load_directory(const char* path) {
    strncpy(current_path, path, sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';
    selected_entry = -1;
    scroll_y = 0;

    /* In production: call fs_listdir and parse results */
    entry_count = 0;

    /* Add parent directory entry if not at root */
    if (strcmp(current_path, "/") != 0) {
        strncpy(entries[entry_count].name, "..", sizeof(entries[0].name));
        entries[entry_count].type = FILE_DIRECTORY;
        entries[entry_count].size = 0;
        entries[entry_count].modified = 0;
        entry_count++;
    }

    /* Stub entries for demonstration */
    const char* stub_names[] = {"Documents", "Pictures", "readme.txt", "config.json"};
    FileType stub_types[] = {FILE_DIRECTORY, FILE_DIRECTORY, FILE_REGULAR, FILE_REGULAR};
    unsigned int stub_sizes[] = {0, 0, 1420, 356};

    for (int i = 0; i < 4 && entry_count < MAX_ENTRIES; i++) {
        strncpy(entries[entry_count].name, stub_names[i], sizeof(entries[0].name));
        entries[entry_count].type = stub_types[i];
        entries[entry_count].size = stub_sizes[i];
        entries[entry_count].modified = 0;
        entry_count++;
    }
}

static void navigate_up(void) {
    char* last_slash = strrchr(current_path, '/');
    if (last_slash && last_slash != current_path) {
        *last_slash = '\0';
        load_directory(current_path);
    } else {
        load_directory("/");
    }
}

static void open_entry(int index) {
    if (index < 0 || index >= entry_count) return;

    if (entries[index].type == FILE_DIRECTORY) {
        if (strcmp(entries[index].name, "..") == 0) {
            navigate_up();
        } else {
            char new_path[512];
            if (strcmp(current_path, "/") == 0) {
                snprintf(new_path, sizeof(new_path), "/%s", entries[index].name);
            } else {
                snprintf(new_path, sizeof(new_path), "%s/%s", current_path, entries[index].name);
            }
            load_directory(new_path);
        }
    } else {
        /* Open file with default app (text editor) */
        /* In production: spawn text_editor.wex with file path */
    }
}

/* ─── File Operations ┼── */

static void delete_selected(void) {
    if (selected_entry < 0 || selected_entry >= entry_count) return;

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entries[selected_entry].name);

    int result = (entries[selected_entry].type == FILE_DIRECTORY)
        ? fs_unlink(full_path)   /* In production: recursive rmdir */
        : fs_unlink(full_path);

    if (result == 0) {
        load_directory(current_path);
    }
}

static void create_folder(void) {
    char name[128] = "New Folder";
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_path, name);

    if (fs_mkdir(full_path) == 0) {
        load_directory(current_path);
    }
}

/* ─── Rendering (stub - in production uses GPU driver) ┼── */

static const char* file_icon(FileType type, const char* name) {
    if (type == FILE_DIRECTORY) return "📁";

    /* Check extension */
    const char* ext = strrchr(name, '.');
    if (!ext) return "📄";
    if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".md") == 0) return "📝";
    if (strcmp(ext, ".json") == 0) return "📋";
    if (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0) return "🖼";
    if (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0) return "🎵";
    if (strcmp(ext, ".wex") == 0) return "⚡";
    if (strcmp(ext, ".Wdll") == 0) return "🔌";
    return "📄";
}

static void format_size(unsigned int bytes, char* out, int out_len) {
    if (bytes == 0) {
        strncpy(out, "--", out_len);
        return;
    }
    if (bytes < 1024) {
        snprintf(out, out_len, "%u B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(out, out_len, "%u KB", bytes / 1024);
    } else {
        snprintf(out, out_len, "%u MB", bytes / (1024 * 1024));
    }
}

/* ─── Main Loop ┼── */

void filemanager_tick(void* arg) {
    /* In production:
     * 1. Poll input events (mouse clicks, keyboard)
     * 2. Handle:
     *    - Sidebar click → navigate to favorite
     *    - Entry double-click → open_entry()
     *    - Entry single-click → select
     *    - Right-click → context menu
     *    - Toolbar buttons (back, forward, up, view toggle)
     * 3. Render:
     *    - Sidebar with favorites (blurred background + noise)
     *    - Toolbar with path breadcrumb and buttons
     *    - File grid/list with icons and names
     *    - Selection highlight (blue accent #3B82F6)
     *    - Status bar with item count
     *    - Frosted glass design language
     * 4. Present via GPU driver
     */
}

int main(int argc, char** argv) {
    current_window = wm_create_window("File Manager", 120, 60, 800, 550);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);

    load_directory("/");

    emscripten_set_interval(filemanager_tick, 16, NULL);

    return 0;
}
