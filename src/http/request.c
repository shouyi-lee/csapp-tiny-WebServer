#include "config.h"
#include "request.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

typedef char* supported_method_t;
supported_method_t supported_methods[]
=
{
    "GET",
    "HEAD"
};

void parse_http_request(http_request_t *hrp)
{
    hrp->is_suport_method = 0;

    if (sscanf(hrp->request_line, "%s %s %s",
        hrp->method, hrp->url, hrp->version) != 3)
        return;

    //log_requestline(hrp->request_line);

    size_t method_list_len = sizeof(supported_methods) / sizeof(supported_method_t);
    for (size_t i = 0; i < method_list_len; i++)
        if (!strcmp(supported_methods[i], hrp->method))
        {
            hrp->is_suport_method = 1;
            break;
        }

    return;
}

void parse_http_request_head(rio_t *rp, int *keep_alive, http_request_t *hrp)
{
    *keep_alive = 1;
    hrp->has_range = 0;

    char buf[BUFLEN] = {};
    char headbuf[BUFLEN] = {};

    do
    {
        if (rio_readlineb(rp, buf, BUFLEN) <= 0)
        {
            *keep_alive = 0;
            return;
        }

        if (strstr(buf, "Connection:")
            && sscanf(buf, "Connection: %s", headbuf) == 1
            && !strcmp(headbuf, "close"))
            *keep_alive = 0;

        if (strstr(buf, "Range: bytes="))
        {
            off_t start = -1, end = -1;
            if (sscanf(buf, "Range: bytes=%ld-%ld", &start, &end) >= 1)
            {
                hrp->has_range = 1;
                hrp->range_start = start;
                hrp->range_end = end;
            }
        }
    }while (strcmp(buf, "\r\n"));

    return;
}