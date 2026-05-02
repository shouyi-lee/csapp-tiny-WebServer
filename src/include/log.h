#ifndef LOG_H
#define LOG_H

#include <sys/types.h>

ssize_t log_customeraddr(const char *hostname, const char *port);
ssize_t log_init();
ssize_t log_requesthead(const char *requsthead);
ssize_t log_requestline(const char* requestline);

#endif