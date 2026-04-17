#ifndef STATIC_H
#define STATIC_H

#include "rio_io.h"
#include "request.h"

void serve_static(rio_t *rp, http_request_t *hrp);

#endif