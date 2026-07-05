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

void parse_http_request_line(rio_t *rp, http_request_t *hrp)
{
    hrp->is_suport_method = 0;
    hrp->is_valid_request = 0;
    hrp->have_receive_request = 0;

    ssize_t read_r = rio_readlineb(rp, hrp->raw_request, BUFLEN);
    if (read_r <= 0 || read_r == BUFLEN - 1)
        return;

    hrp->have_receive_request = 1;

    if (sscanf(hrp->raw_request, "%s %s %s",
        hrp->method, hrp->url, hrp->version) != 3)
        return;

    hrp->is_valid_request = 1;

    size_t method_list_len = sizeof(supported_methods) / sizeof(supported_method_t);
    for (size_t i = 0; i < method_list_len; i++)
        if (!strcmp(supported_methods[i], hrp->method))
        {
            hrp->is_suport_method = 1;
            break;
        }

    return;
}

void parse_http_request_head(rio_t *rp, http_request_t *hrp)
{
    hrp->keep_alive = 1;
    hrp->is_valid_request_head = 1;

    char buf[BUFLEN];
    
    do
    {
        ssize_t read_r = rio_readlineb(rp, buf, BUFLEN);
        if (read_r <= 0 || read_r >= BUFLEN - 1)
        {
            hrp->keep_alive = 0;
            hrp->is_valid_request_head = 0;
        }

        char *cmp_r = strcasestr(buf, "connection");
        if (cmp_r != NULL)
        {
            cmp_r = strcasestr(buf, "close");
            if (cmp_r != NULL)
            {
                hrp->keep_alive = 0;
                break;
            }
        }
        
    }while (strncmp(buf, "\r\n", 3));

    return;
}