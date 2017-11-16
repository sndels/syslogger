#include "log_routine.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "signal_handler.h"

void* log_routine(void* arg)
{
    printf("Opened thread for pipe %s\n", (char*) arg);
    while (!interrupted()) {
        sleep(1);
    }
    printf("Closing thread for pipe %s\n", (char*) arg);
    // TODO: Open pipe, send in sysv to DAEMON_KEY ^ pid
    // Listen pipe, print out (mutex!)
    // Synchronize before exit if no better use for it
    pthread_exit(arg);
}
