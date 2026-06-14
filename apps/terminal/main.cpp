/**
 * WebOS Terminal Application (.wex)
 *
 * A command-line terminal emulator with a built-in shell.
 * Supports command history, line editing, and basic shell builtins.
 * Demonstrates keyboard input handling, text rendering, and
 * filesystem interaction in WebOS.
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

    /* Input driver */
    int input_poll_events(void* buf, int max_count);
    int input_get_key(void);
    int input_get_mouse_x(void);
    int input_get_mouse_y(void);

    /* Filesystem service */
    int fs_open(const char* path, int flags);
    int fs_read(int fd, void* buf, int count);
    int fs_write(int fd, const void* buf, int count);
    int fs_close(int fd);
    int fs_mkdir(const char* path);
    int fs_listdir(const char* path, void* entries, int max_entries);
    int fs_stat(const char* path, void* stat_buf);
    int fs_unlink(const char* path);

    /* Process syscalls */
    #define SYS_exit 0x0200
    #define SYS_getpid 0x0201
    #define SYS_spawn 0x0202
    long syscall0(long n);
    long syscall1(long n, long a1);
    long syscall2(long n, long a1, long a2);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
    int syscall_getpid(void) { return (int)syscall0(SYS_getpid); }
}

/* ─── Terminal State ─── */

static int current_window = 0;
static int term_cols = 80;
static int term_rows = 25;
static int cursor_x = 0;
static int cursor_y = 0;
static int cursor_visible = 1;
static int scroll_offset = 0;

/* Line buffer for current input */
static char input_buf[512] = {0};
static int input_len = 0;
static int input_cursor = 0;

/* Command history */
#define MAX_HISTORY 64
static char history[MAX_HISTORY][256];
static int history_count = 0;
static int history_index = 0;

/* Terminal output buffer (scrollback) */
#define SCROLLBACK_LINES 500
static char screen_buf[SCROLLBACK_LINES][256];
static int screen_line_count = 0;

/* Current working directory */
static char cwd[512] = "/";

/* Prompt string */
static const char* PROMPT = "webos> ";

/* ─── Screen Buffer Operations ─── */

static void screen_write_line(const char* text) {
    if (screen_line_count < SCROLLBACK_LINES) {
        strncpy(screen_buf[screen_line_count], text, 255);
        screen_buf[screen_line_count][255] = '\0';
        screen_line_count++;
    } else {
        /* Scroll up: shift all lines up by one */
        for (int i = 1; i < SCROLLBACK_LINES; i++) {
            memcpy(screen_buf[i - 1], screen_buf[i], 256);
        }
        strncpy(screen_buf[SCROLLBACK_LINES - 1], text, 255);
        screen_buf[SCROLLBACK_LINES - 1][255] = '\0';
    }
}

static void screen_write_fmt(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    screen_write_line(buf);
}

static void write_prompt(void) {
    char prompt_buf[512];
    snprintf(prompt_buf, sizeof(prompt_buf), "%s%s", cwd, PROMPT);
    screen_write_line(prompt_buf);
}

/* ─── Shell Builtin Commands ─── */

static void cmd_help(void) {
    screen_write_line("WebOS Shell v0.0.1beta");
    screen_write_line("Available commands:");
    screen_write_line("  help        - Show this help message");
    screen_write_line("  clear       - Clear the terminal screen");
    screen_write_line("  echo <msg>  - Print a message");
    screen_write_line("  pwd         - Print working directory");
    screen_write_line("  cd <dir>    - Change directory");
    screen_write_line("  ls [dir]    - List directory contents");
    screen_write_line("  mkdir <dir> - Create a directory");
    screen_write_line("  cat <file>  - Display file contents");
    screen_write_line("  rm <file>   - Remove a file");
    screen_write_line("  ps          - List running processes");
    screen_write_line("  uname       - Print system information");
    screen_write_line("  date        - Print current date/time");
    screen_write_line("  whoami      - Print current user");
    screen_write_line("  exit        - Close terminal");
}

static void cmd_clear(void) {
    screen_line_count = 0;
    for (int i = 0; i < SCROLLBACK_LINES; i++) {
        screen_buf[i][0] = '\0';
    }
}

static void cmd_echo(const char* msg) {
    screen_write_line(msg);
}

static void cmd_pwd(void) {
    screen_write_line(cwd);
}

static void cmd_cd(const char* dir) {
    if (!dir || dir[0] == '\0') {
        strncpy(cwd, "/", sizeof(cwd));
        return;
    }
    if (strcmp(dir, "..") == 0) {
        /* Go up one directory */
        char* last_slash = strrchr(cwd, '/');
        if (last_slash && last_slash != cwd) {
            *last_slash = '\0';
        } else {
            strncpy(cwd, "/", sizeof(cwd));
        }
    } else if (dir[0] == '/') {
        /* Absolute path */
        strncpy(cwd, dir, sizeof(cwd) - 1);
        cwd[sizeof(cwd) - 1] = '\0';
    } else {
        /* Relative path */
        int len = strlen(cwd);
        if (len > 1 && cwd[len - 1] != '/') {
            strncat(cwd, "/", sizeof(cwd) - len - 1);
        }
        strncat(cwd, dir, sizeof(cwd) - strlen(cwd) - 1);
    }
}

static void cmd_ls(const char* dir) {
    const char* target = (dir && dir[0] != '\0') ? dir : cwd;
    /* In production: fs_listdir(target, ...) and render entries */
    screen_write_fmt("  (directory listing for %s)", target);
}

static void cmd_cat(const char* path) {
    if (!path || path[0] == '\0') {
        screen_write_line("cat: missing file operand");
        return;
    }
    /* In production: fs_open + fs_read loop + fs_close */
    screen_write_fmt("  (file contents of %s)", path);
}

static void cmd_mkdir(const char* dir) {
    if (!dir || dir[0] == '\0') {
        screen_write_line("mkdir: missing operand");
        return;
    }
    int result = fs_mkdir(dir);
    if (result == 0) {
        screen_write_fmt("  created directory: %s", dir);
    } else {
        screen_write_fmt("  mkdir: failed to create '%s'", dir);
    }
}

static void cmd_rm(const char* path) {
    if (!path || path[0] == '\0') {
        screen_write_line("rm: missing operand");
        return;
    }
    int result = fs_unlink(path);
    if (result != 0) {
        screen_write_fmt("  rm: cannot remove '%s'", path);
    }
}

static void cmd_ps(void) {
    screen_write_line("  PID  STATE     NAME");
    screen_write_line("  1    running   kernel");
    screen_write_line("  2    running   terminal.wex");
}

static void cmd_uname(void) {
    screen_write_line("WebOS 0.0.1beta (WebAssembly/WebGPU)");
}

static void cmd_date(void) {
    /* In production: use SYS_time syscall */
    screen_write_line("  (current date/time from host)");
}

static void cmd_whoami(void) {
    screen_write_line("user");
}

/* ─── Command Execution ─── */

static void execute_command(const char* cmd) {
    /* Skip leading whitespace */
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    if (*cmd == '\0') return;

    /* Add to history */
    if (history_count < MAX_HISTORY) {
        strncpy(history[history_count], cmd, 255);
        history[history_count][255] = '\0';
        history_count++;
    }
    history_index = history_count;

    /* Parse command and arguments */
    char command[64] = {0};
    char args[448] = {0};
    int i = 0;
    while (cmd[i] && cmd[i] != ' ' && i < 63) {
        command[i] = cmd[i];
        i++;
    }
    command[i] = '\0';
    while (cmd[i] == ' ') i++;
    strcpy(args, cmd + i);

    /* Dispatch */
    if (strcmp(command, "help") == 0) { cmd_help(); }
    else if (strcmp(command, "clear") == 0) { cmd_clear(); }
    else if (strcmp(command, "echo") == 0) { cmd_echo(args); }
    else if (strcmp(command, "pwd") == 0) { cmd_pwd(); }
    else if (strcmp(command, "cd") == 0) { cmd_cd(args); }
    else if (strcmp(command, "ls") == 0) { cmd_ls(args); }
    else if (strcmp(command, "mkdir") == 0) { cmd_mkdir(args); }
    else if (strcmp(command, "cat") == 0) { cmd_cat(args); }
    else if (strcmp(command, "rm") == 0) { cmd_rm(args); }
    else if (strcmp(command, "ps") == 0) { cmd_ps(); }
    else if (strcmp(command, "uname") == 0) { cmd_uname(); }
    else if (strcmp(command, "date") == 0) { cmd_date(); }
    else if (strcmp(command, "whoami") == 0) { cmd_whoami(); }
    else if (strcmp(command, "exit") == 0) {
        wm_destroy_window(current_window);
        syscall_exit(0);
    }
    else {
        screen_write_fmt("  %s: command not found", command);
    }
}

/* ─── Input Handling ─── */

static void handle_key(int keycode, int mods) {
    /* Enter: execute command */
    if (keycode == 13) {
        input_buf[input_len] = '\0';
        screen_write_line(input_buf);
        execute_command(input_buf);
        input_len = 0;
        input_cursor = 0;
        input_buf[0] = '\0';
        write_prompt();
        return;
    }

    /* Backspace */
    if (keycode == 8) {
        if (input_cursor > 0) {
            /* Shift characters left */
            for (int i = input_cursor; i < input_len; i++) {
                input_buf[i - 1] = input_buf[i];
            }
            input_len--;
            input_cursor--;
            input_buf[input_len] = '\0';
        }
        return;
    }

    /* Delete */
    if (keycode == 46) {
        if (input_cursor < input_len) {
            for (int i = input_cursor; i < input_len - 1; i++) {
                input_buf[i] = input_buf[i + 1];
            }
            input_len--;
            input_buf[input_len] = '\0';
        }
        return;
    }

    /* Arrow keys */
    if (keycode == 37) { /* Left */
        if (input_cursor > 0) input_cursor--;
        return;
    }
    if (keycode == 39) { /* Right */
        if (input_cursor < input_len) input_cursor++;
        return;
    }
    if (keycode == 38) { /* Up - history */
        if (history_index > 0) {
            history_index--;
            strncpy(input_buf, history[history_index], sizeof(input_buf) - 1);
            input_len = strlen(input_buf);
            input_cursor = input_len;
        }
        return;
    }
    if (keycode == 40) { /* Down - history */
        if (history_index < history_count - 1) {
            history_index++;
            strncpy(input_buf, history[history_index], sizeof(input_buf) - 1);
        } else {
            history_index = history_count;
            input_buf[0] = '\0';
        }
        input_len = strlen(input_buf);
        input_cursor = input_len;
        return;
    }

    /* Home / End */
    if (keycode == 36) { input_cursor = 0; return; }
    if (keycode == 35) { input_cursor = input_len; return; }

    /* Tab completion (basic) */
    if (keycode == 9) {
        /* In production: autocomplete from known commands and file paths */
        return;
    }

    /* Ctrl+C: cancel current input */
    if (keycode == 3) {
        screen_write_line("^C");
        input_len = 0;
        input_cursor = 0;
        input_buf[0] = '\0';
        write_prompt();
        return;
    }

    /* Ctrl+L: clear screen */
    if (keycode == 12) {
        cmd_clear();
        write_prompt();
        return;
    }

    /* Printable character */
    if (keycode >= 32 && keycode < 127 && input_len < (int)sizeof(input_buf) - 1) {
        /* Insert at cursor position */
        for (int i = input_len; i > input_cursor; i--) {
            input_buf[i] = input_buf[i - 1];
        }
        input_buf[input_cursor] = (char)keycode;
        input_len++;
        input_cursor++;
        input_buf[input_len] = '\0';
    }
}

/* ─── Main Loop ─── */

void terminal_tick(void* arg) {
    /* In production:
     * 1. Poll keyboard events from input driver
     * 2. Handle key presses (handle_key)
     * 3. Render terminal screen:
     *    - Background (#0F172A dark)
     *    - Text lines from screen_buf
     *    - Cursor blinking
     *    - Scrollbar
     * 4. Present via GPU driver
     */

    /* Blink cursor */
    static int cursor_blink = 0;
    cursor_blink = (cursor_blink + 1) % 60;
    cursor_visible = cursor_blink < 40;
}

int main(int argc, char** argv) {
    current_window = wm_create_window("Terminal", 100, 80, 720, 480);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);

    /* Welcome message */
    screen_write_line("WebOS Terminal v0.0.1beta");
    screen_write_line("Type 'help' for available commands.");
    screen_write_line("");
    write_prompt();

    emscripten_set_interval(terminal_tick, 16, NULL);

    return 0;
}
