#include "write_routine.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "signal_handler.h"
#include "logmsg_queue.h"

void* write_routine(void* arg)
{
    FILE* log_file = (FILE*) arg;

    // Write queued messages to file
    logmsg* msg;
    char date_buf[16] = {'\0'};
    while ((msg = dequeue_all_logmsgs()) != NULL || !interrupted()) {
        if (msg == NULL) {
            usleep(50000);
            continue;
        } 
        while (msg != NULL) {
            // Wait for possible write in buffer
            pthread_mutex_lock(&msg->mutex);
            pthread_mutex_unlock(&msg->mutex);

            // Format timestamp
            strftime(date_buf, 16, "%b %e %T", gmtime(&msg->time.tv_sec));
            long millis = msg->time.tv_nsec / 1000000;

            // Print formatted log message to file
            fprintf(log_file, "%s.%03ld %d %s %s\n",
                    date_buf, millis, msg->client_pid, msg->client_name, msg->buf);

            // Free resources
            logmsg* next_msg = msg->next;
            free(msg->buf);
            free(msg->client_name);
            free(msg);
            msg = next_msg;
        }
    }

    fclose(log_file);
    return 0;
}
