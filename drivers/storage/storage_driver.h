#ifndef STORAGE_DRIVER_H
#define STORAGE_DRIVER_H

int storage_driver_init(void);
int storage_driver_read(const char* path, void* buf, int buf_len);
int storage_driver_write(const char* path, const void* data, int data_len);
int storage_driver_exists(const char* path);
int storage_driver_delete(const char* path);
int storage_driver_mkdir(const char* path);

#endif
