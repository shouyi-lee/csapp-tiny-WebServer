#include "config.h"
#include "log.h"
#include <unistd.h>
#include <stdio.h>

ssize_t log_customeraddr(const char *hostname, const char *port)
{
    fprintf(stdout, "receive connection form %s:%s\n", hostname, port);
    return 0;
}