/**
 * WebOS File System Service (.Wdll)
 * 
 * Virtual File System (VFS) that provides a unified file API
 * on top of the storage driver. Supports:
 * - Path-based file operations (read, write, delete, stat)
 * - Directory management (mkdir, listdir)
 * - VFS mount points (extensible backend)
 * - Path normalization
 */

#include "fs_service.h"
#include <string.h>

#define MAX_PATH_LEN 256
#define MAX_OPEN_FILES 64
#define MAX_MOUNTS 16

/* Open file descriptor */
typedef struct {
    int fd;
    int owner_pid;
    char path[MAX_PATH_LEN];
    int flags;       /* O_RDONLY, O_WRONLY, O_RDWR */
    int position;
    int in_use;
} open_file_t;

/* VFS mount point */
typedef struct {
    char prefix[MAX_PATH_LEN];
    int driver_id;  /* Reference to storage driver */
} vfs_mount_t;

static open_file_t open_files[MAX_OPEN_FILES];
static vfs_mount_t mounts[MAX_MOUNTS];
static int next_fd = 1;
static int mount_count = 0;

EMSCRIPTEN_KEEPALIVE
int fs_service_init(void) {
    memset(open_files, 0, sizeof(open_files));
    memset(mounts, 0, sizeof(mounts));
    next_fd = 1;
    mount_count = 0;
    
    /* Mount root filesystem */
    vfs_mount_t* root = &mounts[mount_count++];
    strcpy(root->prefix, "/");
    root->driver_id = 1; /* Default storage driver */
    
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int fs_open(int owner_pid, const char* path, int flags) {
    /* Find free descriptor */
    int slot = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].in_use) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return -1; /* Too many open files */
    
    open_file_t* f = &open_files[slot];
    f->fd = next_fd++;
    f->owner_pid = owner_pid;
    f->flags = flags;
    f->position = 0;
    f->in_use = 1;
    
    int len = 0;
    while (path[len] && len < MAX_PATH_LEN - 1) {
        f->path[len] = path[len];
        len++;
    }
    f->path[len] = '\0';
    
    return f->fd;
}

EMSCRIPTEN_KEEPALIVE
int fs_read(int fd, void* buf, int len) {
    open_file_t* f = 0;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fd == fd && open_files[i].in_use) {
            f = &open_files[i];
            break;
        }
    }
    if (!f) return -1;
    
    /* Read via storage driver */
    extern int storage_driver_read(const char*, void*, int);
    int result = storage_driver_read(f->path, buf, len);
    if (result > 0) {
        f->position += result;
    }
    return result;
}

EMSCRIPTEN_KEEPALIVE
int fs_write(int fd, const void* buf, int len) {
    open_file_t* f = 0;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fd == fd && open_files[i].in_use) {
            f = &open_files[i];
            break;
        }
    }
    if (!f) return -1;
    
    extern int storage_driver_write(const char*, const void*, int);
    return storage_driver_write(f->path, buf, len);
}

EMSCRIPTEN_KEEPALIVE
int fs_close(int fd) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].fd == fd) {
            open_files[i].in_use = 0;
            return 0;
        }
    }
    return -1;
}

EMSCRIPTEN_KEEPALIVE
int fs_mkdir(const char* path) {
    extern int storage_driver_mkdir(const char*);
    return storage_driver_mkdir(path);
}

EMSCRIPTEN_KEEPALIVE
int fs_unlink(const char* path) {
    extern int storage_driver_delete(const char*);
    return storage_driver_delete(path);
}

EMSCRIPTEN_KEEPALIVE
int fs_exists(const char* path) {
    extern int storage_driver_exists(const char*);
    return storage_driver_exists(path);
}
