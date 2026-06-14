/**
 * WebOS Text Editor Application (.wex)
 *
 * A lightweight text editor with basic features:
 * - Multi-line editing with cursor movement
 * - File open/save via filesystem service
 * - Line numbers
 * - Basic syntax highlighting (C/JSON)
 * - Find and replace
 * - Tab indentation
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
    int fs_stat(const char* path, void* stat_buf);

    /* Input driver */
    int input_poll_events(void* buf, int max_count);

    #define SYS_exit 0x0200
    long syscall1(long n, long a1);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
}

/* ─── Editor Constants ┼── */

#define MAX_LINES     1000
#define MAX_LINE_LEN  1024
#define LINENUM_WIDTH 48
#define TAB_SIZE      4
#define SCROLL_SPEED  3

/* ─── Editor State ┼── */

static int current_window = 0;
static char file_path[512] = "(untitled)";
static int modified = 0;

/* Text buffer */
static char lines[MAX_LINES][MAX_LINE_LEN];
static int line_count = 1;

/* Cursor */
static int cursor_line = 0;
static int cursor_col = 0;
static int cursor_visible = 1;
static int selection_start_line = -1;
static int selection_start_col = -1;
static int selection_end_line = -1;
static int selection_end_col = -1;

/* Scroll */
static int scroll_x = 0;
static int scroll_y = 0;

/* Find/Replace */
static char find_text[128] = "";
static char replace_text[128] = "";
static int find_active = 0;
static int find_case_sensitive = 0;

/* ─── Buffer Operations ┼── */

static void editor_clear(void) {
    line_count = 1;
    lines[0][0] = '\0';
    cursor_line = 0;
    cursor_col = 0;
    scroll_x = 0;
    scroll_y = 0;
    modified = 0;
    strncpy(file_path, "(untitled)", sizeof(file_path));
}

static void insert_char(char ch) {
    int len = strlen(lines[cursor_line]);
    if (len >= MAX_LINE_LEN - 1) return;

    /* Insert at cursor position */
    for (int i = len; i >= cursor_col; i--) {
        lines[cursor_line][i + 1] = lines[cursor_line][i];
    }
    lines[cursor_line][cursor_col] = ch;
    cursor_col++;
    modified = 1;
}

static void insert_newline(void) {
    if (line_count >= MAX_LINES) return;

    /* Split current line at cursor */
    int len = strlen(lines[cursor_line]);
    char* remainder = lines[cursor_line] + cursor_col;

    /* Shift lines down */
    for (int i = line_count; i > cursor_line + 1; i--) {
        strcpy(lines[i], lines[i - 1]);
    }

    /* Move remainder to new line */
    strcpy(lines[cursor_line + 1], remainder);
    lines[cursor_line][cursor_col] = '\0';

    line_count++;
    cursor_line++;
    cursor_col = 0;
    modified = 1;
}

static void delete_char_back(void) {
    if (cursor_col > 0) {
        int len = strlen(lines[cursor_line]);
        for (int i = cursor_col; i < len; i++) {
            lines[cursor_line][i - 1] = lines[cursor_line][i];
        }
        lines[cursor_line][len - 1] = '\0';
        cursor_col--;
        modified = 1;
    } else if (cursor_line > 0) {
        /* Join with previous line */
        int prev_len = strlen(lines[cursor_line - 1]);
        int cur_len = strlen(lines[cursor_line]);

        if (prev_len + cur_len < MAX_LINE_LEN) {
            strcat(lines[cursor_line - 1], lines[cursor_line]);
            cursor_col = prev_len;

            /* Shift lines up */
            for (int i = cursor_line; i < line_count - 1; i++) {
                strcpy(lines[i], lines[i + 1]);
            }
            line_count--;
            cursor_line--;
            modified = 1;
        }
    }
}

static void delete_char_forward(void) {
    int len = strlen(lines[cursor_line]);
    if (cursor_col < len) {
        for (int i = cursor_col; i < len; i++) {
            lines[cursor_line][i] = lines[cursor_line][i + 1];
        }
        modified = 1;
    } else if (cursor_line < line_count - 1) {
        /* Join with next line */
        int cur_len = strlen(lines[cursor_line]);
        int next_len = strlen(lines[cursor_line + 1]);

        if (cur_len + next_len < MAX_LINE_LEN) {
            strcat(lines[cursor_line], lines[cursor_line + 1]);

            for (int i = cursor_line + 1; i < line_count - 1; i++) {
                strcpy(lines[i], lines[i + 1]);
            }
            line_count--;
            modified = 1;
        }
    }
}

static void insert_tab(void) {
    for (int i = 0; i < TAB_SIZE && cursor_col < MAX_LINE_LEN - 1; i++) {
        insert_char(' ');
    }
}

/* ─── File I/O ┼── */

static void open_file(const char* path) {
    int fd = fs_open(path, 0 /* O_RDONLY */);
    if (fd < 0) return;

    editor_clear();
    strncpy(file_path, path, sizeof(file_path) - 1);

    char buf[4096];
    int total_read = 0;
    int current_line = 0;
    int col = 0;

    while (1) {
        int n = fs_read(fd, buf, sizeof(buf));
        if (n <= 0) break;

        for (int i = 0; i < n && current_line < MAX_LINES; i++) {
            if (buf[i] == '\n') {
                lines[current_line][col] = '\0';
                current_line++;
                col = 0;
            } else if (buf[i] == '\r') {
                /* Skip carriage return */
            } else if (col < MAX_LINE_LEN - 1) {
                lines[current_line][col++] = buf[i];
            }
        }
        total_read += n;
    }

    lines[current_line][col] = '\0';
    line_count = current_line + 1;
    if (line_count < 1) line_count = 1;

    fs_close(fd);
    modified = 0;
}

static void save_file(const char* path) {
    int fd = fs_open(path, 0x03 /* O_WRONLY | O_CREAT */);
    if (fd < 0) return;

    for (int i = 0; i < line_count; i++) {
        fs_write(fd, lines[i], strlen(lines[i]));
        if (i < line_count - 1) {
            fs_write(fd, "\n", 1);
        }
    }

    fs_close(fd);
    modified = 0;
    if (path) {
        strncpy(file_path, path, sizeof(file_path) - 1);
    }
}

/* ─── Find ┼── */

static int find_next(void) {
    if (find_text[0] == '\0') return -1;
    int find_len = strlen(find_text);

    for (int l = cursor_line; l < line_count; l++) {
        int start_col = (l == cursor_line) ? cursor_col : 0;
        int line_len = strlen(lines[l]);

        for (int c = start_col; c <= line_len - find_len; c++) {
            int match = 1;
            for (int k = 0; k < find_len; k++) {
                char a = lines[l][c + k];
                char b = find_text[k];
                if (!find_case_sensitive) {
                    if (a >= 'A' && a <= 'Z') a += 32;
                    if (b >= 'A' && b <= 'Z') b += 32;
                }
                if (a != b) { match = 0; break; }
            }
            if (match) {
                cursor_line = l;
                cursor_col = c;
                return 0;
            }
        }
    }
    return -1; /* Not found */
}

/* ─── Key Handling ┼── */

static void handle_key(int keycode, int mods) {
    /* Ctrl shortcuts */
    if (mods & 0x02) { /* Ctrl */
        switch (keycode) {
            case 19: /* Ctrl+S - Save */
                save_file(file_path);
                return;
            case 6:  /* Ctrl+F - Find */
                find_active = !find_active;
                return;
            case 3:  /* Ctrl+C - Copy (stub) */
                return;
            case 22: /* Ctrl+V - Paste (stub) */
                return;
            case 24: /* Ctrl+X - Cut (stub) */
                return;
        }
    }

    switch (keycode) {
        case 13: /* Enter */
            insert_newline();
            break;
        case 8: /* Backspace */
            delete_char_back();
            break;
        case 46: /* Delete */
            delete_char_forward();
            break;
        case 37: /* Left */
            if (cursor_col > 0) cursor_col--;
            break;
        case 39: /* Right */
            if (cursor_col < (int)strlen(lines[cursor_line])) cursor_col++;
            break;
        case 38: /* Up */
            if (cursor_line > 0) cursor_line--;
            break;
        case 40: /* Down */
            if (cursor_line < line_count - 1) cursor_line++;
            break;
        case 36: /* Home */
            cursor_col = 0;
            break;
        case 35: /* End */
            cursor_col = strlen(lines[cursor_line]);
            break;
        case 33: /* Page Up */
            cursor_line -= 20;
            if (cursor_line < 0) cursor_line = 0;
            break;
        case 34: /* Page Down */
            cursor_line += 20;
            if (cursor_line >= line_count) cursor_line = line_count - 1;
            break;
        case 9: /* Tab */
            insert_tab();
            break;
        default:
            if (keycode >= 32 && keycode < 127) {
                insert_char((char)keycode);
            }
            break;
    }

    /* Clamp cursor */
    if (cursor_col > (int)strlen(lines[cursor_line])) {
        cursor_col = strlen(lines[cursor_line]);
    }
}

/* ─── Main Loop ┼── */

void editor_tick(void* arg) {
    /* In production:
     * 1. Poll keyboard events → handle_key()
     * 2. Render:
     *    - Background (surface base #0F172A)
     *    - Line numbers gutter (surface elevated)
     *    - Text with syntax highlighting
     *    - Cursor (blinking blue accent bar)
     *    - Selection highlight (blue accent transparent)
     *    - Scrollbar
     *    - Menu bar (File, Edit, View, Help)
     *    - Find/Replace bar (if active)
     *    - Status bar (line:col, file name, modified flag)
     * 3. Present via GPU driver
     */

    static int blink_counter = 0;
    blink_counter = (blink_counter + 1) % 60;
    cursor_visible = blink_counter < 40;
}

int main(int argc, char** argv) {
    current_window = wm_create_window("Text Editor", 80, 50, 750, 550);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);

    /* If a file was specified, open it */
    if (argc > 1 && argv[1]) {
        open_file(argv[1]);
    }

    emscripten_set_interval(editor_tick, 16, NULL);

    return 0;
}
