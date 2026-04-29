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

    if (!sscanf(hrp->request_line, "%s %s %s",
        hrp->method, hrp->url, hrp->version))
        return;

    log_requestline(hrp->request_line);

    size_t method_list_len = sizeof(supported_methods) / sizeof(supported_method_t);
    for (size_t i = 0; i < method_list_len; i++)
        if (!strcmp(supported_methods[i], hrp->method))
        {
            hrp->is_suport_method = 1;
            break;
        }

    return;
}

void parse_http_request_head(rio_t *rp)
{
    char buf[BUFLEN] = {};
    rio_readlineb(rp, buf, BUFLEN);

    while (strcmp("\r\n", buf)){
        rio_readlineb(rp, buf, BUFLEN);
        log_requesthead(buf);
    }
}