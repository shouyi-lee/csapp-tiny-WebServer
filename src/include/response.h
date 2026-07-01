#ifndef RESPONSE_H
#define RESPONSE_H

#include <sys/types.h>
#include "rio_io.h"

ssize_t send_responseline(rio_t *rp, const char *version, const char* code, const char *message);
ssize_t clienterror(rio_t *rp, const char *code);
ssize_t send_responsehead(rio_t *rp, const char *head_name, const char *head_data);
ssize_t build_responsehead(char *sendbuf, const char *head_name, const char *head_data, size_t offset);
ssize_t build_responseline(char *sendbuf, const char *version, const char* code, const char *message);

#endif