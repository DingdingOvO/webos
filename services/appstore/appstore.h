#ifndef APPSTORE_H
#define APPSTORE_H

int appstore_init(void);
int appstore_register(const char* name, const char* desc, const char* version,
                      const char* entry, const char* icon, const char* category);
int appstore_get_count(void);
int appstore_find_by_name(const char* name);
int appstore_install(int app_id);
int appstore_uninstall(int app_id);
int appstore_is_installed(int app_id);

#endif
