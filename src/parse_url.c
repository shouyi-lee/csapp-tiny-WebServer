#include "config.h"
#include "parse_url.h"
#include <stdio.h>
#include <string.h>

void parse_url(http_request_t *hrp)
{
    if (!strncmp("/", hrp->url, BUFLEN))
        memcpy(hrp->url, "/index.html", BUFLEN);

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
            strcpy(hrp->cgiargs, ptr);
        }
        else
            strcpy(hrp->cgiargs, "");
    }
    else
        hrp->is_static = 1;

    strcpy(hrp->filename, "./www");
    strcat(hrp->filename, hrp->url);
}