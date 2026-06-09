/**
 * WebOS App Store Service (.Wdll)
 * 
 * Manages application manifests, discovery, download, and installation.
 * Apps are described by app.json manifests and distributed as .wex files
 * with optional .Wdll dependencies.
 */

#include "appstore.h"
#include <string.h>

#define MAX_APPS 128
#define MAX_APP_NAME 64
#define MAX_APP_DESC 256

/* App manifest */
typedef struct {
    int id;
    char name[MAX_APP_NAME];
    char description[MAX_APP_DESC];
    char version[16];
    char entry[128];     /* .wex file path */
    char icon[256];      /* Icon URL */
    char category[32];
    int installed;
    int size;
} app_entry_t;

static app_entry_t apps[MAX_APPS];
static int app_count = 0;
static int next_app_id = 1;

EMSCRIPTEN_KEEPALIVE
int appstore_init(void) {
    memset(apps, 0, sizeof(apps));
    app_count = 0;
    
    /* Register built-in apps */
    appstore_register("Calculator", "Simple calculator", "1.0.0", "/apps/calculator.wex", "/icons/calc.png", "Utilities");
    appstore_register("Paint", "Drawing application", "1.0.0", "/apps/paint.wex", "/icons/paint.png", "Graphics");
    appstore_register("Browser", "Web browser", "1.0.0", "/apps/browser.wex", "/icons/browser.png", "Internet");
    appstore_register("App Store", "Application store", "1.0.0", "/apps/appstore.wex", "/icons/store.png", "System");
    
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int appstore_register(const char* name, const char* desc, const char* version,
                      const char* entry, const char* icon, const char* category) {
    if (app_count >= MAX_APPS) return -1;
    
    app_entry_t* app = &apps[app_count];
    app->id = next_app_id++;
    app->installed = 0;
    
    /* Copy strings safely */
    strncpy(app->name, name, MAX_APP_NAME - 1);
    strncpy(app->description, desc, MAX_APP_DESC - 1);
    strncpy(app->version, version, 15);
    strncpy(app->entry, entry, 127);
    strncpy(app->icon, icon, 255);
    strncpy(app->category, category, 31);
    
    app_count++;
    return app->id;
}

EMSCRIPTEN_KEEPALIVE
int appstore_get_count(void) {
    return app_count;
}

EMSCRIPTEN_KEEPALIVE
int appstore_find_by_name(const char* name) {
    for (int i = 0; i < app_count; i++) {
        if (strcmp(apps[i].name, name) == 0) {
            return apps[i].id;
        }
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
int appstore_install(int app_id) {
    for (int i = 0; i < app_count; i++) {
        if (apps[i].id == app_id) {
            apps[i].installed = 1;
            return 0;
        }
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
int appstore_uninstall(int app_id) {
    for (int i = 0; i < app_count; i++) {
        if (apps[i].id == app_id) {
            apps[i].installed = 0;
            return 0;
        }
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
int appstore_is_installed(int app_id) {
    for (int i = 0; i < app_count; i++) {
        if (apps[i].id == app_id) {
            return apps[i].installed;
        }
    }
    return 0;
}
