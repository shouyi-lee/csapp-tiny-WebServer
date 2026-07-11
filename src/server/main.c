#include "config.h"
#include "server.h"
#include "sigset.h"
#include "log.h"
#include "pool.h"
#include "pack_socket.h"

#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stdout, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    init_signo();

    if (log_init() < 0)
    {
        fprintf(stdout, "init log serve failed\n");
        return -1;
    }

    if (pool_init() < 0)
    {
        fprintf(stdout, "init pool failed\n");
        pool_destroy();
        return -1;
    }

    if (serve_init() < 0)
    {
        fprintf(stdout, "init serve failed\n");
        return -1;
    }
    
    int listenfd = open_listenfd(argv[1]);
    if (listenfd < 0)
    {
        fprintf(stdout, "not a available port\n");
        return -1;
    }

    while (1)
    {
        int confd = accept(listenfd, NULL, NULL);
        if (confd < 0) continue;   

        customer_add(confd);
    }

    return 0;
}