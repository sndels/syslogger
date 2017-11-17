#include "sysv_messaging.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#define DAEMON_KEY 0x12345678

int crt_sysv(key_t client)
{
    key_t key = DAEMON_KEY ^ client;
    // Check that the key is free
    if (msgget(key, 0) != -1) {
        printf("Queue 0x%08x already active\n", key);
        return -1;
    }

    int id;
    if ((id = msgget(key, IPC_CREAT | 0777)) == -1) {
        perror("msgget");
        return -1;
    }
    return id;
}

int snd_sysv(key_t client, long mtype, const char* msg, size_t mlen)
{
    key_t key = DAEMON_KEY ^ client;
    // Check if queue is active
    int id;
    if ((id = msgget(key, 0)) == -1) {
        perror("msget");
        return 1;
    }

    msg_t* mess;

    if ((mess = (msg_t*) malloc(sizeof(long) + mlen)) == NULL) {
        perror("malloc");
        return 1;
    }
    memcpy(mess->mtext, msg, mlen);
    mess->mtype = 1;

    // Send message
    if (msgsnd(id, mess, mlen, 0)) {
        perror("msgsnd");
        return 1;
    }
    return 0;
}

int rcv_sysv(key_t client, long mtype, msg_t** msg, size_t* msize)
{
    key_t key = DAEMON_KEY ^ client;
    // Check if queue is active
    int id;
    if ((id = msgget(key, 0)) == -1) {
        perror("msget");
        return 1;
    }
    int mlen;
    while (1) {
        if ((mlen = msgrcv(id, *msg, *msize, 0, 0)) == -1) {
            if (errno == EINTR) {
                return 1;
            } else if (errno == E2BIG) {
                free(*msg);
                *msize *= 2;
                if ((*msg = (msg_t*) malloc(sizeof(long) + *msize + 1)) == NULL) {
                    perror("malloc");
                    return 1;
                }
                continue;
            } else {
                perror("msgrcv");
                return 1;
            }
        }
        break;
    }

    (*msg)->mtext[mlen] = '\0';
    return 0;
}

int chk_sysv(key_t client)
{
    // TODO: check if message present
    return 0;
}
