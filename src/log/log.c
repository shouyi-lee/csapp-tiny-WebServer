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

extern char log_url[BUFLEN];

static int log_file_fd;
pthread_mutex_t mutex;

/*需要调用者自己保证log_unit合法, 或者说其中每个字符串都有结束字节*/
ssize_t log_request(log_unit_t *log_unit)
{
    char writebuf[BUFLEN];
    if (time_stamp(writebuf) == NULL) return -1;
    size_t time_len = strlen(writebuf);

    int rc1 = snprintf(writebuf + time_len, BUFLEN - time_len, " [info] %s   %s %s -> 200 OK\n", log_unit->source_ip, log_unit->method, log_unit->url);
    if (rc1 >= BUFLEN - (int)time_len) rc1 = snprintf(writebuf + time_len, BUFLEN - time_len, " [error] method or url are too long to show\n");
    if (rc1 < 0) return -1;

    pthread_mutex_lock(&mutex);
    ssize_t rc2 = rio_writen(log_file_fd, writebuf, (size_t)rc1 + time_len);
    pthread_mutex_unlock(&mutex);

    return rc2;
}

//调用者保证log_unit各字段有终止符号，且err_stat不超长
ssize_t log_error(log_unit_t *log_unit)
{
    char writebuf[BUFLEN];
    if (time_stamp(writebuf) == NULL) return -1;
    size_t time_len = strlen(writebuf);

    int rc1 = snprintf(writebuf + time_len, BUFLEN - time_len, " [error] %s   %s   %s %s\n", log_unit->source_ip, log_unit->err_stat, log_unit->method, log_unit->url);
    if (rc1 >= BUFLEN - (int)time_len) rc1 = snprintf(writebuf + time_len, BUFLEN - time_len, " [error] error: %s\tmethod or url are too long to show\n", log_unit->err_stat);
    if (rc1 < 0) return -1;

    pthread_mutex_lock(&mutex);
    ssize_t rc2 = rio_writen(log_file_fd, writebuf, (size_t)rc1 + time_len);
    pthread_mutex_unlock(&mutex);

    return rc2;
}

ssize_t log_init()
{
    int fd = open(log_url, O_CREAT | O_RDWR | O_APPEND, 0644);
    if (fd < 0) return -1;
    log_file_fd = fd;
    return 0;
}