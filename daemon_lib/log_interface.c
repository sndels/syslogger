#include "log_interface.h"

#include <assert.h> // TODO: proper error handling
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/sysv_messaging.h"

const log_client* register_client(const char* name)
{
    log_client* client;
    if ((client = (log_client*) malloc(sizeof(log_client))) == NULL) {
        fprintf(stderr, "Failed to allocate a new log_client\n");
        perror("malloc");
        return NULL;
    }

    pid_t pid = getpid();
    // Open queue for daemon->pid messaging
    if ((client->sysv_queue = crt_sysv(pid)) == -1) {
        free(client);
        return NULL;
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
        free(client);
        return NULL;
    }
    snprintf(msg, mlen, "%s %d", name, pid);

    // Send message
    if (snd_sysv(0, 1, msg, mlen)) {
        free(msg);
        free(client);
        return NULL;
    }
    free(msg);

    // Check for response
    size_t msize = 20; // Allocated space
    msg_t* mess;
    if ((mess = (msg_t*) malloc(sizeof(long) + msize + 1)) == NULL) {
        perror("msg_t malloc");
        free(client);
        return NULL;
    }

    // This will hang if invalid name sent to daemon
    if (rcv_sysv(pid, 0, &mess, &msize)) {
        fprintf(stderr, "Failed receiving sysv message\n");
        free(mess);
        free(client);
        return NULL;
    }

    // Open pipe for write
    if ((client->log_pipe = open(mess->mtext, O_WRONLY)) == -1) 
    {
        fprintf(stderr, "Failed opening FIFO\n");
        perror("open");
        free(mess);
        free(client);
        return NULL;
    }

    free(mess);
    return client;
}

int unregister_client(const log_client* client)
{
    int ret_val = 0;
    // Close pipe
    if (close(client->log_pipe)) {
        fprintf(stderr, "Error closing FIFO\n");
        perror("close");
        ret_val = 1;
    }
    // Close message queue
    if (msgctl(client->sysv_queue, 0, IPC_RMID)) {
        fprintf(stderr, "Error closing sysv queue\n");
        perror("msgctl");
        ret_val = 1;
    }
    free((log_client*) client);
    return ret_val;
}

int log_msg(const log_client* client, const char* msg)
{
    // TODO: check if daemon has exited
    assert(write(client->log_pipe, msg, strlen(msg)) != -1);
    return 0;
}
