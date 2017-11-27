#include "signal_handler.h"

#include <pthread.h>
#include <signal.h>

static int quit = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int interrupted()
{
    pthread_mutex_lock(&mutex);
    int was_quit = quit;
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
