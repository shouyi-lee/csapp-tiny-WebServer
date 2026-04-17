#include "config.h"
#include "rio_io.h"
#include "response.h"
#include "pack_socket.h"
#include "log.h"
#include "request.h"
#include "parse_url.h"
#include "dynamic.h"
#include "static.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>

void doit(int confd)
{
    rio_t rio;
    rio_init(&rio, confd);
    
    http_request_t clientrequest;
    rio_readlineb(&rio, clientrequest.request_line, BUFLEN);
    parse_http_request(&clientrequest);

    if (!clientrequest.is_suport_method)
    {
        clienterror(&rio, "501");
        return;
    }

    parse_http_request_head(&rio);
    parse_url(&clientrequest);

    if (!clientrequest.is_suport_url)
    {
        clienterror(&rio, "404");
        return;
    }

    struct stat filestat;
    stat(clientrequest.filename, &filestat);
    if (!S_ISREG(filestat.st_mode) || !(S_IRUSR & filestat.st_mode))
    {
        clienterror(&rio, "404");
        return;
    }

    if (clientrequest.is_static)
        serve_static(&rio, &clientrequest);
    else
        serve_dynamic(&rio, &clientrequest);
}

void serve(const char *port)
{
    int listenfd = open_listenfd(port);
    if (listenfd < 0)
    {
        fprintf(stdout, "not a available port\n");
        return;
    }

    while (1)
    {
        socklen_t clientlen = sizeof(struct sockaddr_storage);
        struct sockaddr_storage client;
        struct sockaddr *clientp = (struct sockaddr*)&client;

        int confd = accept(listenfd, clientp, &clientlen);
        if (confd < 0) continue;

        char hostname[1024], port[1024];
        getnameinfo(clientp, clientlen, hostname, 1024, port, 1024, NI_NUMERICHOST|NI_NUMERICSERV);   
        log_customeraddr(hostname, port);

        doit(confd);
        close(confd);
    }
}