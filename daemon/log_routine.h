#ifndef LOG_ROUTINE_H
#define LOG_ROUTINE_H

#include <pthread.h>

typedef struct logr_init {
    char* pipe_path;
    int fd;
    int client_pid;
    char* client_name;
} logr_init;

int start_log(pthread_t* thread_id, const char* base_path, char* reg_msg);
void* log_routine(void* arg);

#endif // LOG_ROUTINE_H
