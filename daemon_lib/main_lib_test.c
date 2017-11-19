#include "log_interface.h"

#include <stdlib.h>
#include <stdio.h>

int main()
{
    printf("Registering client\n");
    if (register_client("thread0")) {
        printf("Failed");
        return 1;
    }
    log_msg("this is a message");
    printf("Unregistering client\n");
    if (unregister_client()) {
        printf("Failed");
        return 1;
    }
    printf("Exiting\n");
    return 0;
}
