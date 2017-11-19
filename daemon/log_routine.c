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

int start_log(pthread_t* thread_id, const char* base_path, char* reg_msg)
{
    // Parse client info form register message
    char* name_end = strchr(reg_msg, ' ');
    if (name_end == NULL || name_end == reg_msg) {
        fprintf(stderr, "Invalid message format: %s\n", reg_msg);
        free(reg_msg);
        return 1;
    }
    // Get pid
    int client_pid;
    if (sscanf(name_end + 1, "%d", &client_pid) != 1) {
        fprintf(stderr, "Invalid client pid: %s\n", name_end + 1);
        free(reg_msg);
        return 1;
    }

    size_t base_path_len = strlen(base_path);
    size_t client_len = strlen(reg_msg);
    size_t client_pid_len = strlen(name_end + 1);
    size_t client_name_len = name_end - reg_msg + 1;
    char* pipe_path = (char*) calloc(base_path_len + client_len + 1, sizeof(char));
    memcpy(pipe_path, base_path, base_path_len); // Base path
    memcpy(pipe_path + base_path_len, name_end + 1, client_pid_len); // PID
    memcpy(pipe_path + base_path_len + client_pid_len, reg_msg, client_name_len); // Name

    printf("Opening pipe %s\n", pipe_path);
    if ((mkfifo(pipe_path, S_IRWXU | S_IWGRP| S_IWOTH) == -1)) {
        snd_sysv(client_pid, 1, ERRMSG, strlen(ERRMSG));
        perror("mkfifo");
        free(reg_msg);
        free(pipe_path);
        return 1;
    }

    snd_sysv(client_pid, 1, pipe_path, strlen(pipe_path));

    int fd;
    if ((fd = open(pipe_path, O_RDONLY)) == -1) 
    {
        perror("Failed to open FIFO");
        remove(pipe_path);
        free(reg_msg);
        free(pipe_path);
        return 1;
    }

    // Cut pid from name string
    *name_end = '\0';
    if ((reg_msg = (char*) realloc(reg_msg, client_name_len)) == NULL) {
        fprintf(stderr, "Failed to truncate pid from client name\n");
        perror("realloc");
        free(reg_msg);
        return 1;
    }

    printf("Opening thread for client %d %s\n", client_pid, reg_msg);
    logr_init* arg = (logr_init*) malloc(sizeof(logr_init));
    arg->pipe_path = pipe_path;
    arg->fd = fd;
    arg->client_pid = client_pid;
    arg->client_name = reg_msg;
    pthread_create(thread_id, NULL, log_routine, (void*) arg);

    return 0;
}

void* log_routine(void* arg)
{
    logr_init* info = (logr_init*) arg;
    size_t client_len = strlen(info->client_name);
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
                msg->client_pid = info->client_pid;
                msg->client_name = (char*) malloc(client_len + 1);
                memcpy(msg->client_name, info->client_name, client_len);
                msg->client_name[client_len] = '\0';

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

    printf("Closing thread for client %u %s\n", info->client_pid, info->client_name);

    char* client = info->client_name;
    close(info->fd);
    remove(info->pipe_path);
    free(info->pipe_path);
    free(info);
    pthread_exit(client);
}
