#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../daemon_lib/log_interface.h"

#define THREADS 10

static int quit = 0;
static pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;

static int should_quit()
{
    pthread_mutex_lock(&q_mutex);
    int ret_val = quit;
    pthread_mutex_unlock(&q_mutex);
    return ret_val;
}

static void* test_routine(void* arg)
{
    char name[20];
    snprintf(name, 20, "thread%ld", (long) arg);
    const int client = register_client(name);
    if (client == -1) {
        return (void*) 1;
    }
    snprintf(name, 20, "%d thread%ld", getpid(), (long) arg);

    char date_buf[16] = {'\0'};
    char msg_buf[64] = {'\0'};
    long ret_val = 0;
    printf("thread %ld logging\n", (long) arg);
    while (!should_quit()) {
        // Format timestamp
        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        strftime(date_buf, 16, "%b %e %T", gmtime(&t.tv_sec));
        long millis = t.tv_nsec / 1000000;

        // Print formatted log message to file
        snprintf(msg_buf, 64, "MSG form %s at %s.%03ld", name, date_buf, millis);
        if (log_msg(client, msg_buf)) {
            // Most likely daemon exited
            ret_val = 1;
            break;
        }
        usleep(1000);
    }
    printf("thread %ld finished logging\n", (long) arg);

    if (unregister_client(client)) ret_val = 1;
    return (void*) ret_val;
}

int main()
{
    // Ignore SIGPIPE as required by libdaemon
    signal(SIGPIPE, SIG_IGN);

    // Start test threads
    pthread_t* thread_ids;
    assert((thread_ids = malloc(sizeof(pthread_t) * THREADS)));
    for (long i = 0; i < THREADS; i++) {
        pthread_create(&thread_ids[i], NULL, test_routine, (void*) i);
    }

    // Wait for console input to end test
    printf("Press any key to quit\n");
    getchar();
    pthread_mutex_lock(&q_mutex);
    quit = 1;
    pthread_mutex_unlock(&q_mutex);

    // Wait for test threads to exit
    for (int i = 0; i < THREADS; i++) {
        void* thread_result = NULL;
        assert((pthread_join(thread_ids[i], &thread_result)) == 0);
        printf("Thread %d returned with %lu\n", i, (long) thread_result);
    }
    free(thread_ids);

    return 0;
}
