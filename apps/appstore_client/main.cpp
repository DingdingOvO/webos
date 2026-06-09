/**
 * WebOS App Store Client Application (.wex)
 * 
 * A client for browsing and installing applications from the App Store service.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <emscripten.h>

extern "C" {
    int wm_create_window(const char* title, int x, int y, int width, int height);
    void wm_show_window(int window_id);
    void wm_destroy_window(int window_id);
    
    /* App store service functions */
    int appstore_get_count(void);
    int appstore_find_by_name(const char* name);
    int appstore_install(int app_id);
    int appstore_uninstall(int app_id);
    int appstore_is_installed(int app_id);
    
    #define SYS_exit 0x0200
    long syscall1(long n, long a1);
    void syscall_exit(int code) { syscall1(SYS_exit, code); }
}

static int current_window = 0;
static int selected_app = -1;
static int scroll_offset = 0;

void appstore_client_tick(void* arg) {
    /* In production:
     * 1. Query appstore service for available apps
     * 2. Render app grid/list with icons and descriptions
     * 3. Handle Install/Uninstall button clicks
     * 4. Show app details on selection
     */
}

int main(int argc, char** argv) {
    current_window = wm_create_window("App Store", 100, 50, 700, 550);
    if (current_window <= 0) {
        syscall_exit(1);
    }
    wm_show_window(current_window);
    
    emscripten_set_interval(appstore_client_tick, 16, NULL);
    
    return 0;
}
