#include "config.h"
#include "server.h"
#include "sigset.h"
#include "log.h"
#include "pool.h"
#include "pack_socket.h"

#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/unistd.h>
#include <sys/file.h>

extern server_t server_config;

ssize_t parse_config(char *file_name)
{
    char buf[BUFLEN];
    memset(&server_config, 0, sizeof(server_t));
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t rc;

    while((rc = rio_readline(fd, buf, BUFLEN)) >= 0)
    {
        if (rc == 0)
            break;

        if (!strncmp("log_url", buf, strlen("log_url")))
        {
            if (sscanf(buf, "log_url: %s", server_config.log_url) != 1)
                goto config_error;
            else
                continue;
        }
        else if (!strncmp("source_url", buf, strlen("source_url")))
        {
            if (sscanf(buf, "source_url: %s", server_config.source_url) != 1)
                goto config_error;
            else
                continue;
        }
        else if (!strncmp("root_source", buf, strlen("root_source")))
        {
            if (sscanf(buf, "root_source: %s", server_config.root_source) != 1)
                goto config_error;
            else
                continue;
        }
        else if (!strncmp("port", buf, strlen("port")))
        {
            if (sscanf(buf, "port: %s", server_config.port) != 1)
                goto config_error;
            else
                continue;
        }
        else if (!strncmp("real_ip_head", buf, strlen("real_ip_head")))
        {
            if (sscanf(buf, "real_ip_head: %s", server_config.real_ip_head) != 1)
                goto config_error;
            else
                continue;
        }
        else
            goto config_error;
    }

    if (*server_config.log_url == 0 || *server_config.port == 0
        || *server_config.root_source == 0 || *server_config.source_url == 0
        || *server_config.real_ip_head == 0)
        goto config_error;

    close(fd);
    return 0;

    config_error:
    close(fd);
    return -1;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stdout, "usage: %s <config>\n", argv[0]);
        return -1;
    }

    if (parse_config(argv[1]) < 0)
    {
        fprintf(stdout, "not a valid config file\n");
        return -1;
    }

    init_signo();

    if (log_init() < 0)
    {
        fprintf(stdout, "init log serve failed\n");
        return -1;
    }

    if (pool_init() < 0)
    {
        fprintf(stdout, "init pool failed\n");
        pool_destroy();
        return -1;
    }

    if (serve_init() < 0)
    {
        fprintf(stdout, "init serve failed\n");
        return -1;
    }
    
    int listenfd = open_listenfd(server_config.port);
    if (listenfd < 0)
    {
        fprintf(stdout, "not a available port\n");
        return -1;
    }

    while (1)
    {
        int confd = accept(listenfd, NULL, NULL);
        if (confd < 0) continue;   

        customer_add(confd);
    }

    return 0;
}