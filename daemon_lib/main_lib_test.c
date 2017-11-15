#include "log_interface.h"

#include <stdlib.h>
#include <stdio.h>

int main()
{
    printf("Registering client\n");
    if (registerClient(0)) {
        printf("Failed");
        return 1;
    }
    printf("Exiting\n");
    return 0;
}
