#include "log_interface.h"

#include <stdlib.h>
#include <stdio.h>

int main()
{
    printf("Registering client\n");
    const int client = register_client("thread0");
    if (client == -1) {
        printf("Failed");
        return 1;
    }
    log_msg(client, "this is a message");
    printf("Unregistering client\n");
    if (unregister_client(client)) {
        printf("Failed");
        return 1;
    }
    printf("Exiting\n");
    return 0;
}
