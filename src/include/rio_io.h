#ifndef RIO_IO_H
#define RIO_IO_H

#include <sys/types.h>
#include <sys/select.h>

#define RIO_BUFSIZE 4096

#define RIO_SET(riop, fdsetp) FD_SET((riop)->rio_fd, (fdsetp))
#define RIO_ISSET(riop, fdsetp) FD_ISSET((riop)->rio_fd, (fdsetp))

typedef struct
{
    int rio_fd;
    int rio_cnt;
    char* riobuf_ptr;
    char riobuf[RIO_BUFSIZE];
} rio_t;

ssize_t rio_readn(int fd, char* buf, size_t nbytes);
ssize_t rio_writen(int fd, const char* buf, size_t nbytes);
void rio_init(rio_t *rp, int fd);
ssize_t rio_read(rio_t* rp, char* buf, size_t nbytes);
ssize_t rio_readnb(rio_t* rp, char *buf, size_t nbytes);
ssize_t rio_readlineb(rio_t *rp, char *buf, size_t nbytes);
ssize_t rio_writenb(rio_t *rp, const char *buf, size_t nbytes);
ssize_t rio_deinit(rio_t *rp);

#endif