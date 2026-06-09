#ifndef NET_SERVICE_H
#define NET_SERVICE_H

int net_service_init(void);
int net_http_get(const char* url, void* response_buf, int buf_len);
int net_http_post(const char* url, const void* body, int body_len, void* response_buf);
int net_open_connection(int owner_pid, const char* url);
void net_close_connection(int conn_id);

#endif
