#include "config.h"
#include "parse_url.h"
#include <stdio.h>
#include <sys/stat.h>
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

//健壮的
void parse_url(http_request_t *hrp)
{
    // URL percent-decode first (deepseek-dev feature)
    char decoded[BUFLEN];
    url_decode(decoded, hrp->url);
    memcpy(hrp->url, decoded, BUFLEN);

    hrp->is_valid_url = 0;
    hrp->is_static = 1;

    if (!strcmp("/", hrp->url))
        strcpy(hrp->url, "/index.html");

    // file_level-based path traversal protection (master improvement)
    ssize_t file_level = 0;
    char *neddle1 = hrp->url;
    char *neddle2 = strchr(hrp->url + 1, '/');

    while (neddle2 != NULL)
    {
        if (!strncmp("/..", neddle1, neddle2 - neddle1))
            file_level--;
        else if (strncmp("/.", neddle1, neddle2 - neddle1))
            file_level++;

        neddle1 = neddle2;
        neddle2 = strchr(neddle1 + 1, '/');
    }

    if (!strcmp("/..", neddle1))
        file_level--;
    else if (strcmp("/.", neddle1))
        file_level++;

    if (file_level <= 0)
        return;

    // CGI detection (deepseek-dev feature)
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

    strcpy(hrp->filename, "./website");
    strcat(hrp->filename, hrp->url);

    // stat validation (master improvement)
    struct stat filestat = {};
    if (stat(hrp->filename, &filestat) < 0)
        return;
    if (!S_ISREG(filestat.st_mode) || !(S_IRUSR & filestat.st_mode))
        return;

    hrp->is_valid_url = 1;
}