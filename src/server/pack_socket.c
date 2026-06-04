#include "config.h"
#include "pack_socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <string.h>
#include <sys/errno.h>
#include <netdb.h>

int open_listenfd(const char *port)
{
    struct addrinfo hints, *p, *res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_ADDRCONFIG | AI_NUMERICSERV | AI_PASSIVE;

    int rc = getaddrinfo(NULL, port, &hints, &res);
    if (rc != 0) return -1;

    int listenfd, opt = 1;
    for (p = res; p != NULL; p = p->ai_next)
    {
        listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listenfd < 0)
            continue;
        
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(listenfd);
    }

    freeaddrinfo(res);
    if (p == NULL)
        return -1;

    if (listen(listenfd, 1024) < 0)
    {
        close(listenfd);
        return -1;
    }

    return listenfd;
}