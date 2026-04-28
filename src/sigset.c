#include "config.h"
#include <sys/fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

static void handle_child(int signo)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

ssize_t init_signo()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, handle_child);
    return 0;
}