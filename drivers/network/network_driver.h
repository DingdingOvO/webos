#ifndef NETWORK_DRIVER_H
#define NETWORK_DRIVER_H

int network_driver_init(void);
int network_driver_http_get(const char* url, void* response_buf, int buf_len);
int network_driver_http_post(const char* url, const void* body, int body_len, void* response_buf);

#endif
