#include "log_routine.h"

#include <pthread.h>
#include <stdio.h>

void* log_routine(void* arg)
{
    printf("Opened thread for pipe %s\n", (char*) arg);
    // TODO: Open pipe, send in sysv to DAEMON_KEY ^ pid
    // Listen pipe, print out
    // Synchronization
    // Write to file
    pthread_exit(arg);
}
