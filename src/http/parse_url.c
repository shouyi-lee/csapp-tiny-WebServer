#include "config.h"
#include "parse_url.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

//健壮的
void parse_url(http_request_t *hrp)
{
    hrp->is_valid_url = 0;
    hrp->is_static = 1;

    if (!strcmp("/", hrp->url))
        strcpy(hrp->url, "/index.html");

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

    strcpy(hrp->filename, "./website");
    strcat(hrp->filename, hrp->url);

    struct stat filestat = {};
    if (stat(hrp->filename, &filestat) < 0)
        return;
    if (!S_ISREG(filestat.st_mode) || !(S_IRUSR & filestat.st_mode))
        return;

    hrp->is_valid_url = 1;
}