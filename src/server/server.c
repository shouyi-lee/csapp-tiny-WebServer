#include "config.h"
#include "rio_io.h"
#include "response.h"
#include "pack_socket.h"
#include "log.h"
#include "request.h"
#include "parse_url.h"
#include "dynamic.h"
#include "static.h"
#include "server.h"
#include "pool.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

pthread_t pid[SERVE_THREAD_NUM];

ssize_t doit(rio_t* rp)
{
    http_request_t clientrequest = {};

    parse_http_request_line(rp, &clientrequest);
    if (clientrequest.is_valid_request == 0)
        return -1;

    if (!clientrequest.is_suport_method)
    {
        clienterror(rp, "501");
        return -1;
    }

    parse_http_request_head(rp, &clientrequest);
    if (clientrequest.is_valid_request == 0)
        return -1;

    parse_url(&clientrequest);
    if (!clientrequest.is_valid_url)
    {
        clienterror(rp, "404");
        return 0;
    }

    if (clientrequest.is_static)
        serve_static(rp, &clientrequest);
    else
        serve_dynamic(rp, &clientrequest);

    return clientrequest.keep_alive;
}

void *serve(void *args)
{
    (void) args;
    for (;;)
    {
        task_t task = task_fetch();
        task.keep_alive = doit(&task.customer->rio) <= 0 ? 0 : 1;
        task_return(task);
    }

    return NULL;
}

ssize_t serve_init()
{
    for (size_t i = 0; i < SERVE_THREAD_NUM; i++)
    {
        pthread_create(&pid[i], NULL, serve, NULL);
        pthread_detach(pid[i]);
    }

    return 0;
}