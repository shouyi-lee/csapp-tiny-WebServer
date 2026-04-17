#ifndef RIO_IO_H
#define RIO_IO_H

#include <sys/types.h>

#define RIO_BUFSIZE 4096

typedef struct
{
    int rio_fd;
    int rio_cnt;
    char* riobuf_ptr;
    char riobuf[RIO_BUFSIZE];
} rio_t;

ssize_t rio_readn(int fd, char* buf, size_t nbytes);
ssize_t rio_writen(int fd, char* buf, size_t nbytes);
void rio_init(rio_t *rp, int fd);
ssize_t rio_read(rio_t* rp, char* buf, size_t nbytes);
ssize_t rio_readnb(rio_t* rp, char *buf, size_t nbytes);
ssize_t rio_readlineb(rio_t *rp, char *buf, size_t nbytes);
ssize_t rio_writenb(rio_t *rp, char *buf, size_t nbytes);

#endif