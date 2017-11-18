#include "logmsg_queue.h"

#include <stdio.h>

typedef struct logmsg_queue {
    logmsg* first;
    logmsg* last;
    size_t len;
    pthread_mutex_t mutex;
} logmsg_queue;

static logmsg_queue logmsg_q = { NULL, NULL, PTHREAD_MUTEX_INITIALIZER };

logmsg* create_logmsg()
{
    logmsg* msg;
    if (msg = (logmsg*) malloc(sizeof(logmsg)) == NULL) {
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
    msg->client = NULL;
    msg->buf = NULL;
    msg->next = NULL;
    pthread_mutex_init(&msg->mutex, NULL);
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

logmsg* dequeue_logmsg()
{
    pthread_mutex_lock(&logmsg_q.mutex);

    if (logmsg_q.len == 0) {
        pthread_mutex_unlock(&logmsg_q.mutex);
        return NULL;
    }

    logmsg* first = logmsg_q.first;
    if (logmsg_q.len == 1) {
        logmsg_q.first = NULL;
        logmsg_q.last = NULL;
    } else
        logmsg_q.first = first->next;

    logmsg_q.len--;

    pthread_mutex_unlock(&logmsg_q.mutex);

    // Wait for possible write in buffer
    pthread_mutex_lock(&first->mutex);
    pthread_mutex_unlock(&first->mutex);

    // Set next pointer to null for safety
    first->next = NULL;

    return first;
}
