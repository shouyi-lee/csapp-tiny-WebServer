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
    char raw_request[BUFLEN];
    int is_static;
    int is_suport_method;
    int is_valid_url;
    int has_range;
    off_t range_start;
    off_t range_end;
    int is_valid_request;
    int is_valid_request_head;
    int have_receive_request;
    int keep_alive;
    int is_active;
} http_request_t;

void parse_http_request_line(rio_t *rp, http_request_t *hrp);
void parse_http_request_head(rio_t *rp, http_request_t *hrp);

#endif