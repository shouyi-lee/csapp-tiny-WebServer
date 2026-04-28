#include "config.h"
#include "parse_url.h"
#include <stdio.h>
#include <string.h>

void parse_url(http_request_t *hrp)
{
    hrp->is_suport_method = 1;

    if (!strcmp("/", hrp->url))
        strcpy(hrp->url, "/index.html");

    if (strstr(hrp->url, ".."))
    {
        hrp->is_suport_url = 0;
        return;
    }

    if (strstr(hrp->url, "cgi-bin"))
    {
        hrp->is_static = 0;
        char *ptr = strstr(hrp->url, "?");
        if (ptr)
        {
            *ptr = '\0';
            strcpy(hrp->cgiargs, ptr + 1);
        }
        else
            strcpy(hrp->cgiargs, "");
    }
    else
        hrp->is_static = 1;

    strcpy(hrp->filename, "./website");
    strcat(hrp->filename, hrp->url);
}