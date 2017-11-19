#ifndef LOG_INTERFACE_H
#define LOG_INTERFACE_H

#include <sys/types.h>

// Note: name should not contain spaces or the function will hang
int register_client(const char* name);
int unregister_client();
int log_msg(const char* msg);

#endif // LOG_INTERFACE_H
