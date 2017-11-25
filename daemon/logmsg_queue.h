#ifndef LOGMSG_QUEUE_H
#define LOGMSG_QUEUE_H

#include <pthread.h>
#include <time.h>

typedef struct logmsg {
    struct timespec time;
    int client_pid;
    char* client_name;
    char* buf;
    struct logmsg* next;
    pthread_mutex_t mutex;
} logmsg;

logmsg* create_logmsg();
void queue_logmsg(logmsg* msg);
logmsg* dequeue_all_logmsgs();

#endif // LOGMSG_QUEUE_H
