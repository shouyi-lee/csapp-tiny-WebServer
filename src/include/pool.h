#ifndef POOL_H
#define POOL_H

#include "rio_io.h"
#include "config.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>


typedef struct 
{
    rio_t rio;
    struct sockaddr_storage client_info;
    int alive;
} customer_t;

typedef struct
{
    customer_t *customer;
} task_t;

#define TASK_NUM 16
#define CUSTOMER_NUM 256

#endif