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

//健壮的
void parse_http_request_line(rio_t *rp, http_request_t *hrp)
{
    hrp->is_suport_method = 0;
    hrp->is_valid_request = 0;
    hrp->have_receive_request = 0;

    ssize_t read_r = rio_readlineb(rp, hrp->raw_request, BUFLEN);
    if (read_r <= 0)
        return;
    hrp->have_receive_request = 1;

    if (read_r == BUFLEN - 1)
        return;

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

//健壮的，就是可能陷死在里面，但显然很难避免
void parse_http_request_head(rio_t *rp, http_request_t *hrp)
{
    hrp->keep_alive = 1;
    hrp->is_valid_request_head = 1;
    hrp->get_ip = 0;

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
            }
        }

        ssize_t cmp_s = strncmp(buf, REAL_IP_HEAD, strlen(REAL_IP_HEAD));
        if (!cmp_s)
        {
            char *value_start = strchr(buf, ':') + 1;
            if (value_start == NULL) continue;
            while (*value_start == ' ' || *value_start == '\t') value_start++;

            ssize_t rc = sscanf(value_start, "%s", hrp->raw_ip);
            if (rc == 1) hrp->get_ip = 1;
        }
        
    }while (strncmp(buf, "\r\n", 3));

    return;
}