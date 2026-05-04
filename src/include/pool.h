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
    int used;
    int dealing;
    pthread_mutex_t mutex;
} customer_t;

typedef struct
{
    customer_t *customer;
    int keep_alive;
} task_t;

ssize_t pool_init();
void *task_search(void *args);
ssize_t task_register(customer_t *customer);
task_t task_fetch();
ssize_t task_return(task_t task);
ssize_t customer_add(int fd, void *client_info, size_t client_info_len);
ssize_t customer_delete(customer_t *customer);
ssize_t pool_deinit();

#define TASK_NUM 16
#define CUSTOMER_NUM 256

#endif