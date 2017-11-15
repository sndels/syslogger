#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "daemon_messaging.h"

static int quit = 0;
static void sig_int(int signo)
{
    if (signo == SIGINT) quit = 1;
}

int main(int argc, const char* argv[])
{
    // Init signal handling
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sig_int;
    sigaction(SIGINT, &sig, NULL);

    key_t key = DAEMON_KEY;
    int id;
    typedef struct {
        long mtype;
        char mtext[1];
    } msg_t;

    size_t msize; // Allocated space
    msg_t* mess;
    int mlen;

    // Check that the key is free
    if (msgget(key, 0) != -1) {
        printf("Queue 0x%08x already active\n", key);
        return 1;
    }

    printf("Creating message queue\n");
    if ((id = msgget(key, IPC_CREAT | 0777)) == -1) {
        perror("msgget");
        return 1;
    }

    printf("Allocating initial message buffer\n");
    msize = 20;
    if ((mess = (msg_t*) malloc(sizeof(long) + msize + 1)) == NULL) {
        perror("malloc");
        if ((msgctl(id, 0, IPC_RMID)) != -1)
            perror("msgctl");
        return 1;
    }

    printf("Syslogger ready to receive messages.\n");

    int exit_status = EXIT_SUCCESS;
    do {
        // Wait for messages
        if ((mlen = msgrcv(id, mess, msize, 0, 0)) == -1) {
            if (errno == EINTR) {
                break;
            } else if (errno == E2BIG) {
                free(mess);
                msize *= 2;
                printf("%d message length -> %d\n", getpid(), (int) msize);
                if ((mess = (msg_t*) malloc(sizeof(long) + msize + 1)) == NULL) {
                    perror("malloc");
                    exit_status = EXIT_FAILURE;
                    break;
                }
                continue;
            } else
                perror("msgrcv");
        }

        mess->mtext[mlen] = '\0';
        printf("Received type %ld msg '%s'\n", mess->mtype, mess->mtext);
        // TODO: Register client, thread -pair
        // TODO: Thread initializes and manages pipe

    } while (!quit);

    printf("Exiting\n");

    free(mess);
    if ((msgctl(id, 0, IPC_RMID)) != -1) {
        perror("msgctl");
        exit_status = EXIT_FAILURE;
    }
    return exit_status;
}
