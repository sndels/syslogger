#include "logmsg_queue.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct logmsg_queue {
    logmsg* first;
    logmsg* last;
    size_t len;
    pthread_mutex_t mutex;
} logmsg_queue;

static logmsg_queue logmsg_q = { NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER };

logmsg* create_logmsg()
{
    logmsg* msg;
    if ((msg = (logmsg*) malloc(sizeof(logmsg))) == NULL) {
        fprintf(stderr, "Allocating new logmsg failed\n");
        perror("malloc");
        return NULL;
    }
    if (clock_gettime(CLOCK_REALTIME, &msg->time)) {
        fprintf(stderr, "Allocating new logmsg failed\n");
        perror("clock_gettime");
        free(msg);
        return NULL;
    }
    int ret_val = pthread_mutex_init(&msg->mutex, NULL);
    if (ret_val) {
        fprintf(stderr, "Initializing mutex returned error %d\n", ret_val);
        free(msg);
        return NULL;
    }
    msg->client_pid = 0;
    msg->client_name = NULL;
    msg->buf = NULL;
    msg->next = NULL;
    return msg;
}

void queue_logmsg(logmsg* msg)
{
    pthread_mutex_lock(&logmsg_q.mutex);
    if (logmsg_q.len == 0) {
        logmsg_q.first = msg;
        logmsg_q.last = msg;
    } else {
        logmsg_q.last->next = msg;
        logmsg_q.last = msg;
    }
    logmsg_q.len++;
    pthread_mutex_unlock(&logmsg_q.mutex);
}

logmsg* dequeue_all_logmsgs()
{
    pthread_mutex_lock(&logmsg_q.mutex);

    if (logmsg_q.len == 0) {
        pthread_mutex_unlock(&logmsg_q.mutex);
        return NULL;
    }

    // Dequeue the whole linked list
    logmsg* first = logmsg_q.first;

    logmsg_q.first = NULL;
    logmsg_q.last = NULL;
    logmsg_q.len = 0;

    pthread_mutex_unlock(&logmsg_q.mutex);

    return first;
}
