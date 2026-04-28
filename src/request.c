#include "config.h"
#include "request.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

void parse_http_request(http_request_t *hrp)
{
    hrp->is_suport_method = 1;

    if (!sscanf(hrp->request_line, "%s %s %s",
        hrp->method, hrp->url, hrp->version))
    {
        hrp->is_suport_method = 0;
        return;
    }

    log_requestline(hrp->request_line);

    if (strcasecmp(hrp->method, "GET"))
        hrp->is_suport_method = 0;

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