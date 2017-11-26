#include "log_interface.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/sysv_messaging.h"

static pthread_mutex_t reg_mutex = PTHREAD_MUTEX_INITIALIZER;

const int register_client(const char* name)
{
    pthread_mutex_lock(&reg_mutex);

    pid_t pid = getpid();
    // Open queue for daemon->pid messaging
    int sysv_queue;
    if ((sysv_queue = crt_sysv(pid)) == -1) {
        pthread_mutex_unlock(&reg_mutex);
        return -1;
    }
    // Reserve message with given type and pid
    /* snprintf(NULL, 0,...) will return number of characters in formatteds string
     * %+d will print sign so actual size is one less
     */
    size_t mlen = strlen(name) + 1 + snprintf(NULL, 0, "%+d", pid);
    char* msg;
    if ((msg = malloc(mlen)) == NULL) {
        perror("msg_t malloc");
        if (msgctl(pid, 0, IPC_RMID))
            perror("msgctl");
        pthread_mutex_unlock(&reg_mutex);
        return -1;
    }
    snprintf(msg, mlen, "%s %d", name, pid);

    // Send message
    if (snd_sysv(0, 1, msg, mlen)) {
        pthread_mutex_unlock(&reg_mutex);
        free(msg);
        return -1;
    }
    free(msg);

    // Check for response
    size_t msize = 20; // Allocated space
    msg_t* mess;
    if ((mess = (msg_t*) malloc(sizeof(long) + msize + 1)) == NULL) {
        pthread_mutex_unlock(&reg_mutex);
        perror("msg_t malloc");
        return -1;
    }

    // This will hang if invalid name sent to daemon
    if (rcv_sysv(pid, 0, &mess, &msize)) {
        pthread_mutex_unlock(&reg_mutex);
        fprintf(stderr, "Failed receiving sysv message\n");
        free(mess);
        return -1;
    }

    // Close message queue
    if (msgctl(sysv_queue, 0, IPC_RMID)) {
        pthread_mutex_unlock(&reg_mutex);
        fprintf(stderr, "Error closing sysv queue\n");
        free(mess);
        return -1;
    }
    pthread_mutex_unlock(&reg_mutex);

    // Open pipe for write
    int log_pipe;
    if ((log_pipe = open(mess->mtext, O_WRONLY)) == -1) 
    {
        fprintf(stderr, "Failed opening FIFO\n");
        perror("open");
        free(mess);
        return -1;
    }

    free(mess);
    return log_pipe;
}

int unregister_client(const int log_file)
{
    int ret_val = 0;
    // Close pipe
    if (close(log_file)) {
        fprintf(stderr, "Error closing FIFO\n");
        perror("close");
        ret_val = 1;
    }
    return ret_val;
}

int log_msg(const int log_file, const char* msg)
{
    if (write(log_file, msg, strlen(msg)) == -1) {
        fprintf(stderr, "Write to log file failed\n");
        perror("write");
        return 1;
    }
    return 0;
}
