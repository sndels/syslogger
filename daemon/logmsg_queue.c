#include "logmsg_queue.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct ticket_lock {
    int next_ticket;
    volatile int now_serving;
    pthread_mutex_t next_lock;
} ticket_lock;

int get_ticket(ticket_lock* l)
{
    pthread_mutex_lock(&l->next_lock);
    int ticket = l->next_ticket++;
    pthread_mutex_unlock(&l->next_lock);
    return ticket;
}

typedef struct logmsg_queue {
    logmsg* first;
    logmsg* last;
    size_t len;
    ticket_lock lock;
} logmsg_queue;

static logmsg_queue logmsg_q = { NULL, NULL, 0, {0, 0, PTHREAD_MUTEX_INITIALIZER} };

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
    msg->client_pid = 0;
    msg->client_name = NULL;
    msg->buf = NULL;
    msg->next = NULL;
    pthread_mutex_init(&msg->mutex, NULL);
    return msg;
}

void queue_logmsg(logmsg* msg)
{
    int ticket = get_ticket(&logmsg_q.lock);
    while (ticket != logmsg_q.lock.now_serving);

    if (logmsg_q.len == 0) {
        logmsg_q.first = msg;
        logmsg_q.last = msg;
    } else {
        logmsg_q.last->next = msg;
        logmsg_q.last = msg;
    }
    logmsg_q.len++;

    logmsg_q.lock.now_serving++;
}

logmsg* dequeue_logmsg()
{
    int ticket = get_ticket(&logmsg_q.lock);
    while (ticket != logmsg_q.lock.now_serving);

    if (logmsg_q.len == 0) {
        logmsg_q.lock.now_serving++;
        return NULL;
    }

    logmsg* first = logmsg_q.first;
    if (logmsg_q.len == 1) {
        logmsg_q.first = NULL;
        logmsg_q.last = NULL;
    } else
        logmsg_q.first = first->next;

    logmsg_q.len--;

    logmsg_q.lock.now_serving++;

    // Wait for possible write in buffer
    pthread_mutex_lock(&first->mutex);
    pthread_mutex_unlock(&first->mutex);

    // Set next pointer to null for safety
    first->next = NULL;

    return first;
}
