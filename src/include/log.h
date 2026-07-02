#ifndef LOG_H
#define LOG_H

#include <sys/types.h>

typedef struct 
{
    char *source_ip;
    char *method;
    char *url;
    char *file_stat;
    char *file_length;
} log_unit_t;

ssize_t log_request(log_unit_t *log_unit);
ssize_t log_init();

#endif