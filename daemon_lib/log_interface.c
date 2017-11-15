#include "log_interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "../daemon/daemon_messaging.h"

int registerClient()
{
    key_t key = DAEMON_KEY;
    pid_t client_pid = getpid();
    int id;
    typedef struct {
        long mtype;
        char mtext[1];
    } msg_t;

    size_t mlen;
    msg_t* mess;

    // TODO: Create new queue for this pid
    // key | pid: receive type 1, send type 2 perhaps?

    // Check if daemon queue is active
    if ((id = msgget(key, 0)) == -1) {
        perror("msget");
        return 1;
    }

    // Reserve message with given type and pid
    /* snprintf(NULL, 0,...) will return number of characters in formatteds string
     * %+d will print sign so actual size is one less
     */
    mlen = snprintf(NULL, 0, "%+d", client_pid);
    if ((mess = (msg_t*) malloc(sizeof(long) + mlen)) == NULL) {
        perror("malloc");
        return 1;
    }
    snprintf(mess->mtext, mlen, "%d", client_pid);
    mess->mtype = DAEMON_REGISTER;

    // Send message
    if (msgsnd(id, mess, mlen, 0)) {
        perror("msgsnd");
        free(mess);
        return 1;
    }
    free(mess);

    // TODO: Check for response

    return 0;
}

int unregisterClient()
{
    // TODO: close pipe to send eof signaling client closing
    // TODO: close queue for messaging this pid
    return 0;
}
