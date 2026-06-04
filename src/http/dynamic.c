#include "config.h"
#include "dynamic.h"
#include "response.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void serve_dynamic(rio_t *rp, http_request_t *hrp)
{
    char *argv[] = {hrp->filename, NULL};

    send_responseline(rp, "HTTP/1.1", "200", "OK");
    send_responsehead(rp, "Server", "shouyi-lee Web Server");

    pid_t pid = fork();
    if (pid < 0)
    {
        clienterror(rp, "500");
        return;
    }
    if (pid == 0)
    {
        int maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd < 0) maxfd = 1024;
        for (int fd = 3; fd < maxfd; fd++)
        {
            if (fd != rp->rio_fd)
                close(fd);
        }
        setenv("QUERY_STRING", hrp->cgiargs, 1);
        dup2(rp->rio_fd, STDOUT_FILENO);
        close(rp->rio_fd);
        execve(hrp->filename, argv, environ);
        exit(1);
    }
}