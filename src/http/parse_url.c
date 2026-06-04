#include "config.h"
#include "parse_url.h"
#include <stdio.h>
#include <string.h>

static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static void url_decode(char *dst, const char *src)
{
    while (*src)
    {
        if (*src == '%')
        {
            int hi = hex_val(src[1]);
            int lo = hex_val(src[2]);
            if (hi >= 0 && lo >= 0)
            {
                char c = (char)((hi << 4) | lo);
                if (c == '\0') { src += 3; continue; }
                *dst++ = c;
                src += 3;
                continue;
            }
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

void parse_url(http_request_t *hrp)
{
    char decoded[BUFLEN];
    url_decode(decoded, hrp->url);
    memcpy(hrp->url, decoded, BUFLEN);

    if (!strcmp("/", hrp->url))
        strcpy(hrp->url, "/index.html");

    if (strstr(hrp->url, ".."))
    {
        hrp->is_suport_url = 0;
        return;
    }
    else
        hrp->is_suport_url = 1;

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