#include "config.h"
#include "parse_url.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

static inline int in_hex_char(char ch)
{
    if (ch < '0') return 0;
    if (ch <= '9') return ch;
    if (ch < 'A') return 0;
    if (ch <= 'F') return ch;
    if (ch < 'a') return 0;
    if (ch <= 'f') return ch;
    return 0;
}

static inline char hex_2_val(char ch)
{
    if (ch < 'A') return ch - '0';
    if (ch < 'a') return ch - 'A' + 10;
    return ch - 'a' + 10;
}

static inline ssize_t hex_2_char(char *hex, char *buf)
{
    if (hex[1] == 0 || hex[2] == 0)
        return -1;

    if (!in_hex_char(hex[1]) || !in_hex_char(hex[2]))
        return -1;

    char tem = hex_2_val(hex[2]);
    tem |= hex_2_val(hex[1]) << 4;

    *buf = tem;
    return 0;
}

static ssize_t escape_url(char *url, char *escaped_url, size_t buflen)
{
    size_t write_length = 0;
    char *neddle1 = url, *neddle2 = strchr(url, '%');

    while (neddle2 && neddle1)
    {
        if (write_length + (size_t)(neddle2 - neddle1) + 1 >= buflen)
            return -1;
        strncpy(escaped_url + write_length, neddle1, neddle2 - neddle1);
        write_length += neddle2 - neddle1;

        if (hex_2_char(neddle2, escaped_url + write_length) < 0) return -1;
        write_length++;
        neddle1 = neddle2 + 3;
        neddle2 = strchr(neddle1, '%');
    }

    size_t last_len = strlen(neddle1);
    if (write_length + last_len >= buflen) return -1;
    strcpy(escaped_url + write_length, neddle1);
    return 0;
}

//健壮的
void parse_url(http_request_t *hrp)
{
    hrp->is_valid_url = 0;
    hrp->is_static = 1;

    if (!strcmp("/", hrp->url))
        strcpy(hrp->url, "/index.html");

    char escaped_url[BUFLEN];
    if (escape_url(hrp->url, escaped_url, BUFLEN) < 0) return;
    strcpy(hrp->url, escaped_url);

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
    if (strlen(hrp->filename) + strlen(hrp->url) >= BUFLEN) return;
    strcat(hrp->filename, hrp->url);

    struct stat filestat = {};
    if (stat(hrp->filename, &filestat) < 0)
        return;
    if (!S_ISREG(filestat.st_mode) || !(S_IRUSR & filestat.st_mode))
        return;

    hrp->is_valid_url = 1;
}