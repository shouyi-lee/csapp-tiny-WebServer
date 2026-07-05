#include "config.h"
#include "log.h"
#include "rio_io.h"
#include "time_stamp.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>

static int log_file_fd;
pthread_mutex_t mutex;

/*需要调用者自己保证log_unit合法, 或者说总长度不会大于buflen。log_unit的file_length字段如果响应码非200则为'-'字符*/
ssize_t log_request(log_unit_t *log_unit)
{
    char writebuf[BUFLEN];
    if (time_stamp(writebuf) == NULL) return -1;
    size_t time_len = strlen(writebuf);

    int rc1 = sprintf(writebuf + time_len, " [info] %s %s -> %s\n", log_unit->method, log_unit->url, "200 OK");
    if (rc1 < 0) return -1;

    pthread_mutex_lock(&mutex);
    ssize_t rc2 = rio_writen(log_file_fd, writebuf, (size_t)rc1 + time_len);
    pthread_mutex_unlock(&mutex);

    return rc2;
}

ssize_t log_error(log_unit_t *log_unit)
{
    char writebuf[BUFLEN];
    if (time_stamp(writebuf) == NULL) return -1;
    size_t time_len = strlen(writebuf);

    int rc1 = sprintf(writebuf + time_len, " [error]: %s \t\t%s %s\n", log_unit->err_stat, log_unit->method, log_unit->url);
    if (rc1 < 0) return -1;

    pthread_mutex_lock(&mutex);
    ssize_t rc2 = rio_writen(log_file_fd, writebuf, (size_t)rc1 + time_len);
    pthread_mutex_unlock(&mutex);

    return rc2;
}

ssize_t log_init()
{
    int fd = open("./log_file/server_log", O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd < 0) return -1;
    log_file_fd = fd;
    return 0;
}