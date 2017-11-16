#include "signal_handler.h"

#include <pthread.h>
#include <signal.h>

// TODO: Make these atomic, check quit in threads as well
static int quit = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int interrupted()
{
    int was_quit;
    pthread_mutex_lock(&mutex);
    was_quit = quit;
    pthread_mutex_unlock(&mutex);
    return was_quit;
}

void sig_int(int signo)
{
    if (signo == SIGINT) {
        pthread_mutex_lock(&mutex);
        quit = 1;
        pthread_mutex_unlock(&mutex);
    }
}
