#ifndef LOG_INTERFACE_H
#define LOG_INTERFACE_H

#include <sys/types.h>

int registerClient();
int unregisterClient();
int logMessage(const char* msg, size_t mlen);

#endif // LOG_INTERFACE_H
