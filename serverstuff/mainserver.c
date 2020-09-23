#include <unistd.h>
#include <string.h>
#include "server.h"


int main(int ac, char *ag[])
{
    if (ac > 1 && strcmp(ag[1], "--daemon") == 0)
        daemon(0, 0);
    return start_server();
}
