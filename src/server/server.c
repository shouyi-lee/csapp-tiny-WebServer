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

ssize_t doit(rio_t* rp, int *reuse)
{
    http_request_t clientrequest;
    log_unit_t log = {
        .err_stat = "-",
        .method = "-",
        .url = "-"
    };
    *reuse = 1;

    parse_http_request_line(rp, &clientrequest);
    if (clientrequest.is_valid_request == 0)
    {
        if (clientrequest.have_receive_request)
        {
            log.err_stat = "invalid request";
            log.method = clientrequest.raw_request;
            (void)log_error(&log);
        }
        *reuse = 0;
        return -1;
    }
    log.method = clientrequest.method;
    log.url = clientrequest.url;

    if (!clientrequest.is_suport_method)
    {
        clienterror(rp, "501");
        *reuse = 0;
        log.err_stat = "unsuported method";
        (void)log_error(&log);
        return -1;
    }

    parse_http_request_head(rp, &clientrequest);
    if (clientrequest.is_valid_request_head == 0)
    {
        log.err_stat = "invalid request head";
        (void)log_error(&log);
        *reuse = 0;
        return -1;
    }
    *reuse = clientrequest.keep_alive;

    parse_url(&clientrequest);
    if (!clientrequest.is_valid_url)
    {
        clienterror(rp, "404");
        log.err_stat = "invalid url";
        (void)log_error(&log);
        return -1;
    }

    ssize_t serve_rc = serve_static(rp, &clientrequest);

    if (!clientrequest.is_valid_url)
    {
        clienterror(rp, "404");
        log.err_stat = "invalid url";
        (void)log_error(&log);
        return -1;
    }

    if (serve_rc < 0)
    {
        log.err_stat = "send error";
        (void)log_error(&log);
        *reuse = 0;
        return -1;
    }

    log_request(&log);

    return 0;
}

void *serve(void *args)
{
    (void) args;
    for (;;)
    {
        task_t task = task_acquire();
        ssize_t rc = doit(&task.customer->rio, &task.reuse);
        (void)rc;
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