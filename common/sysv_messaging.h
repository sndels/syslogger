#ifndef SYSV_MESSAGING_H
#define SYSV_MESSAGING_H

#include <sys/msg.h>

typedef struct msg_t {
    long mtype;
    char mtext[1];
} msg_t;

int crt_sysv(key_t client);
int snd_sysv(key_t client, long mtype, const char* msg, size_t mlen);
int rcv_sysv(key_t client, long mtype, msg_t** msg, size_t* mlen);
int chk_sysv(key_t client);

#endif // SYSV_MESSAGING_H
