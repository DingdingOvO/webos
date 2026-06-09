#ifndef FS_SERVICE_H
#define FS_SERVICE_H

/* File flags */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#define O_CREAT  4
#define O_TRUNC  8

int fs_service_init(void);
int fs_open(int owner_pid, const char* path, int flags);
int fs_read(int fd, void* buf, int len);
int fs_write(int fd, const void* buf, int len);
int fs_close(int fd);
int fs_mkdir(const char* path);
int fs_unlink(const char* path);
int fs_exists(const char* path);

#endif
