#include "signal_handler.h"

#include <pthread.h>
#include <signal.h>

static int quit = 0;
/*
 * Mutex is not really needed as only one thread will handle the signal and
 * ints are guaranteed to give either the value before or after write for reading
 * threads. But then again, using one really doesn't hurt either.
 */
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
