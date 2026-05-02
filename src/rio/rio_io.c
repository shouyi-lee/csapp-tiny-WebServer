#include "config.h"
#include "rio_io.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>

ssize_t rio_readn(int fd, const char* buf, size_t nbytes)
{
    ssize_t rc, wait_read;
    wait_read = nbytes;
    char *cbuf = (char*)buf;

    while (wait_read > 0)
    {
        rc = read(fd, cbuf + nbytes - wait_read, wait_read);
        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (rc == 0)
            break;

        wait_read -= rc;
    }

    return nbytes - wait_read;
}

ssize_t rio_writen(int fd, const char* buf, size_t nbytes)
{
    ssize_t wc, wait_write;
    wait_write = nbytes;

    while (wait_write > 0)
    {
        wc = write(fd, buf + nbytes - wait_write, wait_write);
        if (wc < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (wc == 0)
            break;

        wait_write -= wc;
    }

    return nbytes - wait_write;
}

void rio_init(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->riobuf_ptr = rp->riobuf;
}

ssize_t rio_read(rio_t* rp, const char* buf, size_t nbytes)
{
    char *cbuf = (char*)buf;

    while (rp->rio_cnt <= 0)
    {
        rp->rio_cnt = read(rp->rio_fd, rp->riobuf, nbytes);
        if (rp->rio_cnt < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (rp->rio_cnt == 0)
            return 0;

        rp->riobuf_ptr = rp->riobuf;
    }

    int cnt = (signed)nbytes > rp->rio_cnt ? rp->rio_cnt : (signed)nbytes;
    memcpy(cbuf, rp->riobuf_ptr, cnt);
    rp->rio_cnt -= cnt;
    rp->riobuf_ptr += cnt;

    return cnt;
}

ssize_t rio_readnb(rio_t* rp, const char *buf, size_t nbytes)
{
    ssize_t rc, wait_read;
    wait_read = nbytes;

    while (wait_read > 0)
    {
        rc = rio_read(rp, buf + nbytes - wait_read, wait_read);
        if (rc < 0)
            return -1;
        if (rc == 0)
            break;

        wait_read -= rc;
    }

    return nbytes - wait_read;
}

ssize_t rio_readlineb(rio_t *rp, const char *buf, size_t nbytes)
{
    ssize_t rc, wait_read;
    wait_read = nbytes;
    char tem, *cbuf = (char*)buf;

    while (wait_read > 1)
    {
        rc = rio_read(rp, &tem, 1);
        if (rc < 0)
            return -1;
        if (rc == 0)
        {
            if (nbytes - wait_read > 0)
                break;
            return 0;
        }

        cbuf[nbytes -wait_read] = tem;
        wait_read -= rc;

        if (tem == '\n')
            break;
    }

    cbuf[nbytes - wait_read] = '\0';
    return nbytes - wait_read;
}

ssize_t rio_writenb(rio_t *rp, const char *buf, size_t nbytes)
{
    return rio_writen(rp->rio_fd, buf, nbytes);
}