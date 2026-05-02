#include "config.h"
#include "dynamic.h"
#include "response.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void serve_dynamic(rio_t *rp, http_request_t *hrp)
{
    char *argv[] = {NULL};

    send_responseline(rp, "HTTP/1.0", "200", "OK");
    send_responsehead(rp, "Server", "shouyi-lee Web Server");
    
    if (fork() == 0)
    {
        setenv("QUERY_STRING", hrp->cgiargs, 1);
        dup2(rp->rio_fd, STDOUT_FILENO);
        execve(hrp->filename, argv, environ);
        
        clienterror(rp, "404");
        exit(0);
    }
}