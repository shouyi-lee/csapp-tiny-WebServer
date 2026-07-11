#ifndef LOG_H
#define LOG_H

#include <sys/types.h>

typedef struct 
{
    char *method;
    char *url;
    char *err_stat;
    char *source_ip;
} log_unit_t;

ssize_t log_request(log_unit_t *log_unit);
ssize_t log_error(log_unit_t *log_unit);
ssize_t log_init();

#endif