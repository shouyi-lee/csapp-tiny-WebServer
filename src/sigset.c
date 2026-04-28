#include "config.h"
#include <sys/fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <signal.h>

ssize_t init_signo()
{
    signal(SIGPIPE, SIG_IGN);
    return 0;
}