#ifndef LOG_INTERFACE_H
#define LOG_INTERFACE_H

#include <sys/types.h>

int register_client();
int unregister_client();
int log_msg(const char* msg);

#endif // LOG_INTERFACE_H
