#ifndef LOG_INTERFACE_H
#define LOG_INTERFACE_H

#include <sys/types.h>

typedef struct log_client {
    int sysv_queue;
    int log_pipe;
} log_client;

// Note: name should not contain spaces or the function will hang
const log_client* register_client(const char* name);
int unregister_client(const log_client* client);
int log_msg(const log_client* client, const char* msg);

#endif // LOG_INTERFACE_H
