#include <assert.h> // TODO: Actual error handling
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include "daemon_messaging.h"
#include "log_routine.h"
#include "signal_handler.h"

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

    size_t reserved_ids = 2;
    int used_ids = 0;
    pthread_t* thread_ids;
    assert((thread_ids = malloc(sizeof(pthread_t) * reserved_ids)));
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

        // Create log routine for client
        size_t pid_len = strlen(mess->mtext) + 1;
        char* arg = (char*) malloc(pid_len);
        memcpy(arg, mess->mtext, pid_len);
        // Reserve more thread ids if necessary
        if (used_ids == reserved_ids) {
            reserved_ids *= 2;
            assert((thread_ids = realloc(thread_ids, sizeof(pthread_t) * reserved_ids)));
        }
        pthread_create(&thread_ids[used_ids], NULL, log_routine, (void*) arg);
        used_ids++;

        printf("Threads:\n");
        for (int i = 0; i < used_ids; i++)
            printf("%x\n", thread_ids[i]);

    } while (!interrupted());

    printf("Waiting for threads to finish\n");
    for (int i = 0; i < used_ids; i++) {
        void* thread_result = NULL;
        assert((pthread_join(thread_ids[i], &thread_result)) == 0);
        printf("Thread %d returned with %s\n", i, (char*) thread_result);
        free(thread_result);
    }
    free(thread_ids);

    printf("Exiting\n");

    free(mess);
    if ((msgctl(id, 0, IPC_RMID)) != -1) {
        perror("msgctl");
        exit_status = EXIT_FAILURE;
    }
    return exit_status;
}
