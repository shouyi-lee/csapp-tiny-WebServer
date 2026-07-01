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
#include <sys/sendfile.h>

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

static ssize_t get_file_info(http_request_t *hrp, char *filetype, size_t *filelength)
{
    struct stat filestat;
    const char *filename = hrp->filename;

    if (stat(filename, &filestat) != 0) return -1;
    if (filestat.st_size < 0 || (size_t)filestat.st_size > __SIZE_MAX__) return -1;
    *filelength = filestat.st_size;
    
    const char *dot = strrchr(filename, '.');
    const char *ext = dot ? dot : "";

    size_t count = sizeof(mime_type_map) / sizeof(mime_type_map[0]);//手动维护映射表的情况下一般没有问题
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(ext, mime_type_map[i].ext) == 0) {
            strcpy(filetype, mime_type_map[i].mime_type);
            return 0;
        }
    }

    strcpy(filetype, default_mime_type);
    return 0;
}

//需要调用者保证 hrp合法，例如文件名长度不超过buflen。本函数是健壮的吗？依赖于BUFLEN需要足够大，因为未做消息超长处理。
ssize_t serve_static(rio_t *rp, http_request_t *hrp)
{
    char filetype[BUFLEN];
    size_t filelength;
    if (get_file_info(hrp, filetype, &filelength) < 0) goto file_get_error;

    int fd = open(hrp->filename, O_RDONLY);
    if (fd < 0) goto file_get_error;
    
    char sendbuf[BUFLEN];
    char filelength_buf[BUFLEN];
    size_t offset = 0;
    sprintf(filelength_buf, "%zu", filelength);

    offset += build_responseline(sendbuf, "HTTP/1.1", "200", NULL);
    offset += build_responsehead(sendbuf, "Connection", "keep-alive", offset);
    offset += build_responsehead(sendbuf, "Server", "Shouyi-Lee Web Server", offset);
    offset += build_responsehead(sendbuf, "Content-Type", filetype, offset);
    offset += build_responsehead(sendbuf, "Content-Length", filelength_buf, offset);
    offset += build_responsehead(sendbuf, NULL, NULL, offset);

    if (rio_writenb(rp, sendbuf, offset) < 0) return -1;

    if (!strcmp(hrp->method, "HEAD"))
        return 0;

    ssize_t send_rc = sendfile(rp->rio_fd, fd, NULL, filelength);
    close(fd);
    return send_rc > 0 ? 0 : send_rc;

    file_get_error:
    ssize_t error_rc = clienterror(rp, "404");
    return error_rc;
}