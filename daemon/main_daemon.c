#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <unistd.h>

#include "../common/sysv_messaging.h"
#include "log_routine.h"
#include "write_routine.h"
#include "signal_handler.h"

int main(int argc, const char* argv[])
{
    // Init signal handling
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sig_int;
    sigaction(SIGINT, &sig, NULL);

    int id;
    if ((id = crt_sysv(0)) == -1) return 1;

    // Get absolute path to executable
    char* exe_path;
    if ((exe_path = (char*) calloc(PATH_MAX + 1, sizeof(char))) == NULL) {
        fprintf(stderr, "Failed to allocate memory for daemon absolute path\n");
        perror("calloc");
        return 1;
    }
    if (readlink("/proc/self/exe", exe_path, PATH_MAX) == -1) {
        fprintf(stderr, "Could not obtain absolute path to daemon\n");
        perror("readlink");
        free(exe_path);
        return 1;
    }
    size_t path_len = strlen(exe_path);

    // Define log path
    char* log_path;
    if ((log_path = (char*) calloc(PATH_MAX + 1, sizeof(char))) == NULL) {
        fprintf(stderr, "Failed to allocate memory for log absolute path\n");
        perror("calloc");
        free(exe_path);
        return 1;
    }
    memcpy(log_path, exe_path, path_len);
    if (path_len < PATH_MAX - 4)
        memcpy(log_path + path_len, ".log", 4);
    else
        memcpy(log_path + PATH_MAX - 4, ".log", 4);

    // Open log file
    FILE* log_file;
    log_file = fopen (log_path, "a");
    free(log_path);
    if (log_file == NULL)
    {
        fprintf(stderr, "Error opening log file\n");
        perror("fopen");
        if (msgctl(id, 0, IPC_RMID))
            perror("msgctl");
        free(exe_path);
        return 1;
    }

    // Start write thread
    pthread_t write_thread_id;
    pthread_create(&write_thread_id, NULL, write_routine, (void*) log_file);

    msg_t* mess;
    size_t msize = 20; // Allocated space
    if ((mess = (msg_t*) malloc(sizeof(long) + msize + 1)) == NULL) {
        fprintf(stderr, "Failed to allocate the initial message buffer\n");
        perror("malloc");
        if (msgctl(id, 0, IPC_RMID))
            perror("msgctl");
        fclose(log_file);
        free(exe_path);
        return 1;
    }

    printf("Syslogger ready to receive messages.\n");

    size_t reserved_ids = 1;
    int used_ids = 0;
    pthread_t* thread_ids;
    if ((thread_ids = malloc(sizeof(pthread_t) * reserved_ids)) == NULL) {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        perror("malloc");
        free(exe_path);
        return 1;
    }
    int exit_status = EXIT_SUCCESS;
    do {
        // Wait for messages
        if (rcv_sysv(0, 0, &mess, &msize)) break;

        // Copy message to another buffer
        size_t reg_msg_len = strlen(mess->mtext) + 1;
        char* reg_msg = (char*) malloc(reg_msg_len);
        memcpy(reg_msg, mess->mtext, reg_msg_len);

        // Reserve more thread ids if necessary
        if (used_ids == reserved_ids) {
            reserved_ids *= 2;
            if ((thread_ids = realloc(thread_ids, sizeof(pthread_t) * reserved_ids)) == NULL) {
                fprintf(stderr, "Failed to allocate memory for threads\n");
                perror("malloc");
                break;
            }
        }

        // Start new thread for logging
        if (start_log(&thread_ids[used_ids], exe_path, reg_msg) == 0)
            used_ids++;
    } while (!interrupted());
    free(exe_path);
    free(mess);

    printf("Waiting for client threads to finish\n");
    for (int i = 0; i < used_ids; i++) {
        void* thread_result = NULL;
        int ret_val = pthread_join(thread_ids[i], &thread_result);
        if (ret_val)
            fprintf(stderr, "Thread %d join returned with error %d\n", i, ret_val);
        else
            printf("Thread %d returned with %ld\n", i, (long) thread_result);
    }
    free(thread_ids);

    printf("Waiting for log thread to finish\n");
    void* thread_result = NULL;
    int ret_val = pthread_join(write_thread_id, &thread_result);
    if (ret_val)
        fprintf(stderr, "Writer thread join returned with error %d\n", ret_val);
    else
        printf("Writer thread returned with %ld\n", (long) thread_result);

    printf("Exiting\n");

    if (msgctl(id, 0, IPC_RMID)) {
        perror("msgctl");
        exit_status = EXIT_FAILURE;
    }
    return exit_status;
}
