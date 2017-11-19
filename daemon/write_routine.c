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
    while ((msg = dequeue_logmsg()) != NULL || !interrupted()) {
        if (msg != NULL) {
            // Format timestamp
            strftime(date_buf, 16, "%b %e %T", gmtime(&msg->time.tv_sec));
            long millis = msg->time.tv_nsec / 1000000;

            // Print formatted log message to file
            fprintf(log_file, "%s.%03ld %s %s\n", date_buf, millis, msg->client, msg->buf);

            // Free resources
            free(msg->buf);
            free(msg->client);
            free(msg);
        } else
            usleep(50000);
    }

    fclose(log_file);
    return 0;
}
