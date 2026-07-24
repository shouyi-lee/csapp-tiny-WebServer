#ifndef CONFIG_H
#define CONFIG_H

#define _GNU_SOURCE
#define BUFLEN 4096
#define DEFAULTPORT 8080

typedef struct 
{
    char source_url[BUFLEN];
    char log_url[BUFLEN];
    char port[BUFLEN];
    char root_source[BUFLEN];
    char real_ip_head[BUFLEN];
} server_t;


#endif