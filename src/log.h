#ifndef LOG_H
#define LOG_H

#include <sys/types.h>

ssize_t log_customeraddr(const char *hostname, const char *port);
ssize_t log_init();

#endif