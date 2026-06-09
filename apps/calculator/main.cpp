/**
 * WebOS Calculator Application (.wex)
 * 
 * A simple calculator with a graphical window.
 * Demonstrates how to write a C++ application for WebOS using
 * the system call interface and window manager client library.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <emscripten.h>

/* System call stubs - provided by libs/syscall.h */
extern "C" {
    long syscall0(long n);
    long syscall1(long n, long a1);
    long syscall2(long n, long a1, long a2);
    long syscall3(long n, long a1, long a2, long a3);
    
    /* Window manager client functions */
    int wm_create_window(const char* title, int x, int y, int width, int height);
    void wm_show_window(int window_id);
    void wm_destroy_window(int window_id);
    
    /* System calls */
    #define SYS_exit 0x0200
    #define SYS_getpid 0x0201
    #define SYS_debug_log 0x0700
    
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
    int syscall_getpid(void) { return (int)syscall0(SYS_getpid); }
}

/* Calculator state */
static int current_window = 0;
static double display_value = 0.0;
static double stored_value = 0.0;
static char operation = 0;
static int new_number = 1;
static char display_text[32] = "0";

/* Button layout */
struct Button {
    const char* label;
    int x, y, w, h;
    const char* action; /* "digit:N", "op:+", "eq", "clear" */
};

static Button buttons[] = {
    {"7", 10, 80, 65, 45, "digit:7"},
    {"8", 80, 80, 65, 45, "digit:8"},
    {"9", 150, 80, 65, 45, "digit:9"},
    {"/", 220, 80, 65, 45, "op:/"},
    {"4", 10, 130, 65, 45, "digit:4"},
    {"5", 80, 130, 65, 45, "digit:5"},
    {"6", 150, 130, 65, 45, "digit:6"},
    {"*", 220, 130, 65, 45, "op:*"},
    {"1", 10, 180, 65, 45, "digit:1"},
    {"2", 80, 180, 65, 45, "digit:2"},
    {"3", 150, 180, 65, 45, "digit:3"},
    {"-", 220, 180, 65, 45, "op:-"},
    {"0", 10, 230, 135, 45, "digit:0"},
    {".", 150, 230, 65, 45, "digit:."},
    {"+", 220, 230, 65, 45, "op:+"},
    {"C", 10, 280, 100, 45, "clear"},
    {"=", 115, 280, 170, 45, "eq"},
};

static int num_buttons = sizeof(buttons) / sizeof(buttons[0]);

void handle_digit(const char* d) {
    if (new_number) {
        if (d[0] == '.') {
            strcpy(display_text, "0.");
        } else {
            strcpy(display_text, d);
        }
        new_number = 0;
    } else {
        if (d[0] == '.' && strchr(display_text, '.')) return;
        int len = strlen(display_text);
        if (len < 15) {
            display_text[len] = d[0];
            display_text[len + 1] = '\0';
        }
    }
    display_value = atof(display_text);
}

void handle_op(char op) {
    if (operation && !new_number) {
        /* Evaluate pending operation */
        switch (operation) {
            case '+': display_value = stored_value + display_value; break;
            case '-': display_value = stored_value - display_value; break;
            case '*': display_value = stored_value * display_value; break;
            case '/': 
                if (display_value != 0) display_value = stored_value / display_value;
                break;
        }
        snprintf(display_text, sizeof(display_text), "%.10g", display_value);
    }
    stored_value = display_value;
    operation = op;
    new_number = 1;
}

void handle_equals(void) {
    if (operation) {
        switch (operation) {
            case '+': display_value = stored_value + display_value; break;
            case '-': display_value = stored_value - display_value; break;
            case '*': display_value = stored_value * display_value; break;
            case '/':
                if (display_value != 0) display_value = stored_value / display_value;
                break;
        }
        operation = 0;
        snprintf(display_text, sizeof(display_text), "%.10g", display_value);
        new_number = 1;
    }
}

void handle_clear(void) {
    display_value = 0.0;
    stored_value = 0.0;
    operation = 0;
    new_number = 1;
    strcpy(display_text, "0");
}

/* Main loop tick - called by emscripten_set_interval */
void calc_tick(void* arg) {
    /* In a real implementation, this would:
     * 1. Poll for input events
     * 2. Render the calculator UI using GPU driver
     * 3. Handle button clicks
     */
}

int main(int argc, char** argv) {
    /* Create calculator window */
    current_window = wm_create_window("Calculator", 200, 200, 300, 340);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);
    
    /* Setup periodic rendering */
    emscripten_set_interval(calc_tick, 16, NULL); /* ~60fps */
    
    return 0;
}
