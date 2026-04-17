#include "config.h"
#include "static.h"
#include "response.h"
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    const char *ext;
    const char *mime_type;
} MimeTypeMap;

static const MimeTypeMap mime_type_map[] = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
};

static const char *default_mime_type = "application/octet-stream";

static void get_file_info(http_request_t *hrp, char *filetype, char *filelenth)
{
    struct stat filestat;
    stat(hrp->filename, &filestat);
    sprintf(filelenth, "%ld", (long)filestat.st_size);

    const char *filename = hrp->filename;
    const char *dot = strrchr(filename, '.');
    const char *ext = dot ? dot : "";

    size_t count = sizeof(mime_type_map) / sizeof(mime_type_map[0]);
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(ext, mime_type_map[i].ext) == 0) {
            strcpy(filetype, mime_type_map[i].mime_type);
            return;
        }
    }

    strcpy(filetype, default_mime_type);
}

void serve_static(rio_t *rp, http_request_t *hrp)
{
    int fd = open(hrp->filename, O_RDONLY);
    if (fd < 0) return;

    char filetype[BUFLEN];
    char filelenth_b[BUFLEN];
    get_file_info(hrp, filetype, filelenth_b);
    size_t filelenth = atoi(filelenth_b);
    send_responseline(rp, "HTTP/1.0", "200", "OK");
    send_responsehead(rp, "Server", "shouyi-lee Web Server");
    send_responsehead(rp, "Connection", "Close");
    send_responsehead(rp, "Content-length", filelenth_b);
    send_responsehead(rp, "Content-type", filetype);
    send_responsehead(rp, NULL, NULL);

    char *src = mmap(NULL, filelenth, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    rio_writenb(rp, src, filelenth);
    munmap(src, filelenth);
}