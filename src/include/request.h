#ifndef REQUEST_H
#define REQUEST_H
#include "config.h"
#include "rio_io.h"
#define REQUEST_HEAD_NUM 48

typedef struct {
    char filename[BUFLEN];
    char url[BUFLEN];
    char method[BUFLEN];
    char version[BUFLEN];
    char cgiargs[BUFLEN];
    int is_static;
    int is_suport_method;
    int is_suport_url;
    int is_valid_request;
    int keep_alive;
} http_request_t;

void parse_http_request_line(int fd, http_request_t *hrp);
void parse_http_request_head(int fd, http_request_t *hrp);

#endif