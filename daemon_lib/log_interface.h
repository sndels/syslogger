#ifndef LOG_INTERFACE_H
#define LOG_INTERFACE_H

#include <sys/types.h>

// Note: name should not contain spaces or the function will hang
const int register_client(const char* name);
int unregister_client(const int client);
int log_msg(const int client, const char* msg);

#endif // LOG_INTERFACE_H
