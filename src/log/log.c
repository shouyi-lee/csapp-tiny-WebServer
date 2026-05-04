#include "config.h"
#include "log.h"
#include "rio_io.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <string.h>

static rio_t log_file;

ssize_t log_customeraddr(const char *hostname, const char *port)
{
    char buf[BUFLEN];
    sprintf(buf, "receive connection form %s:%s\n", hostname, port);
    rio_writenb(&log_file, buf, strlen(buf));
    return 0;
}

ssize_t log_requestline(const char* requestline)
{
    return rio_writenb(&log_file, requestline, strlen(requestline));
}

ssize_t log_requesthead(const char *requsthead)
{
    return rio_writenb(&log_file, requsthead, strlen(requsthead));
}

ssize_t log_init()
{
    int fd = open("./log_file/server_log", O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd < 0) return -1;
    rio_init(&log_file, fd);
    return 0;
}