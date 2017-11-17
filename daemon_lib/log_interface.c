#include "log_interface.h"

#include <assert.h> // TODO: proper error handling
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../common/sysv_messaging.h"

static int id = -1;
static int fd = -1;

int register_client()
{
    pid_t client = getpid();
    // Open queue for daemon->client messaging
    if ((id = crt_sysv(client)) == -1) {
        return 1;
    }
    // Reserve message with given type and pid
    /* snprintf(NULL, 0,...) will return number of characters in formatteds string
     * %+d will print sign so actual size is one less
     */
    size_t mlen = snprintf(NULL, 0, "%+d", client);
    char* msg;
    if ((msg = malloc(mlen)) == NULL) {
        perror("msg_t malloc");
        if (msgctl(client, 0, IPC_RMID))
            perror("msgctl");
        return 1;
    }
    snprintf(msg, mlen, "%d", client);

    // Send message
    if (snd_sysv(0, 1, msg, mlen)) {
        free(msg);
        return 1;
    }
    free(msg);

    // Check for response
    size_t msize = 20; // Allocated space
    msg_t* mess;
    if ((mess = (msg_t*) malloc(sizeof(long) + msize + 1)) == NULL) {
        perror("msg_t malloc");
        return 1;
    }

    if (rcv_sysv(client, 0, &mess, &msize)) return 1;

    // Open pipe for write
    if ((fd = open(mess->mtext, O_WRONLY)) == -1) 
    {
        perror("Failed to open FIFO");
        free(mess);
        return 1;
    }

    free(mess);
    return 0;
}

int unregister_client()
{
    // Close pipe
    close(fd);
    // Close message queue
    if (msgctl(id, 0, IPC_RMID)) {
        perror("msgctl");
        return 1;
    }
    return 0;
}

int log_msg(const char* msg)
{
    // TODO: check if daemon has exited
    assert(write(fd, msg, strlen(msg)) != -1);
    return 0;
}
