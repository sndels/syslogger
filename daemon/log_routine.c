#include "log_routine.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logmsg_queue.h"
#include "signal_handler.h"
#include "../common/sysv_messaging.h"

#define BUFSIZE 64
#define ERRMSG "opening pipe failed"
#define QUITMSG "daemon quit"

int start_log(pthread_t* thread_id, const char* base_path, char* client)
{
    // Get client id as long
    printf("Converting client id to log\n");
    long client_long;
    sscanf(client, "%li", &client_long);

    printf("Concatenating pipe path\n");
    size_t base_path_len = strlen(base_path);
    size_t client_len = strlen(client);
    char* pipe_path = (char*) malloc(base_path_len + client_len + 1);
    memcpy(pipe_path, base_path, base_path_len);
    memcpy(pipe_path + base_path_len, client, client_len + 1);


    printf("Opening pipe %s\n", pipe_path);
    if ((mkfifo(pipe_path, S_IRWXU | S_IWGRP| S_IWOTH) == -1)) {
        snd_sysv(client_long, 1, ERRMSG, strlen(ERRMSG));
        perror("mkfifo");
        free(client);
        free(pipe_path);
        return 1;
    }

    printf("Sending pipe path to client\n");
    snd_sysv(client_long, 1, pipe_path, strlen(pipe_path));

    printf("Opening pipe for read\n");
    int fd;
    if ((fd = open(pipe_path, O_RDONLY)) == -1) 
    {
        perror("Failed to open FIFO");
        remove(pipe_path);
        free(client);
        free(pipe_path);
        return 1;
    }

    printf("Opening thread for client %s\n", client);
    logr_init* arg = (logr_init*) malloc(sizeof(logr_init));
    arg->pipe_path = pipe_path;
    arg->fd = fd;
    arg->client = client;
    pthread_create(thread_id, NULL, log_routine, (void*) arg);

    return 0;
}

void* log_routine(void* arg)
{
    logr_init* info = (logr_init*) arg;
    size_t client_len = strlen(info->client);
    // Listen pipe
    printf("Listeing pipe\n");
    char buf[BUFSIZE];
    size_t buf_len;
    size_t queued_len;
    int new_read = 1;
    logmsg* msg;
    while (!interrupted()) {
        buf_len = read(info->fd, buf, BUFSIZE);
        buf[buf_len] = '\0';
        if (buf_len > 0) {// TODO: Handle messages longer than BUFSIZE as one
            if (new_read) {
                // Create and queue new log message
                msg = create_logmsg();
                pthread_mutex_lock(&msg->mutex);
                queue_logmsg(msg);// Should be locked to preven premature access

                // Copy client name to message
                msg->client = (char*) malloc(client_len + 1);
                memcpy(msg->client, info->client, client_len);
                msg->client[client_len] = '\0';

                // Copy received buffer to message
                msg->buf = (char*) malloc(buf_len + 1);
                memcpy(msg->buf, buf, buf_len);
                queued_len = buf_len;
            } else {
                // Copy received buffer to message
                msg->buf = (char*) realloc(msg->buf, queued_len + buf_len + 1);
                memcpy(msg->buf + queued_len, buf, buf_len);
                queued_len += buf_len;
            }
            msg->buf[queued_len] = '\0';

            // Only start a new message if read did fit in buffer
            if (buf_len < BUFSIZE) {
                // Unlock message
                pthread_mutex_unlock(&msg->mutex);
                new_read = 1;
            } else 
                new_read = 0;
        } else
            break;
    }

    printf("Closing thread for client %s\n", info->client);

    if (interrupted()) {
        long client;
        sscanf(info->client, "%li", &client);
        snd_sysv(client, 1, QUITMSG, strlen(QUITMSG));
    }
    char* client = info->client;
    close(info->fd);
    remove(info->pipe_path);
    free(info->pipe_path);
    free(info);
    pthread_exit(client);
}
