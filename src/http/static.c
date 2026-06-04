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
#include <errno.h>

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
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".wav", "audio/wav"},
    {".ogg", "audio/ogg"},
    {".flac", "audio/flac"},
    {".aac", "audio/aac"},
    {".m4a", "audio/mp4"},
    {".wma", "audio/x-ms-wma"},
    {".aiff", "audio/aiff"},
    {".webm", "audio/webm"},
    {".opus", "audio/opus"},
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
    char filetype[BUFLEN];
    char filelenth_b[BUFLEN];
    get_file_info(hrp, filetype, filelenth_b);
    off_t filelenth = (off_t)atol(filelenth_b);

    off_t range_start = 0, range_end = filelenth - 1, content_length = filelenth;
    int send_partial = 0;

    if (hrp->has_range)
    {
        if (hrp->range_start >= 0 && hrp->range_end >= 0)
        {
            range_start = hrp->range_start;
            range_end = hrp->range_end < filelenth ? hrp->range_end : filelenth - 1;
            content_length = range_end - range_start + 1;
            send_partial = 1;
        }
        else if (hrp->range_start >= 0)
        {
            range_start = hrp->range_start;
            range_end = filelenth - 1;
            content_length = filelenth - range_start;
            send_partial = 1;
        }
        if (range_start >= filelenth || content_length <= 0)
            send_partial = 0;
    }

    if (send_partial)
    {
        char range_hdr[BUFLEN];
        snprintf(range_hdr, BUFLEN, "bytes %ld-%ld/%ld",
                 range_start, range_end, filelenth);
        send_responseline(rp, "HTTP/1.1", "206", "Partial Content");
        send_responsehead(rp, "Server", "shouyi-lee Web Server");
        send_responsehead(rp, "Connection", "keep-alive");
        send_responsehead(rp, "Accept-Ranges", "bytes");
        send_responsehead(rp, "Content-Range", range_hdr);
        snprintf(filelenth_b, BUFLEN, "%ld", content_length);
        send_responsehead(rp, "Content-length", filelenth_b);
        send_responsehead(rp, "Content-type", filetype);
        send_responsehead(rp, NULL, NULL);
    }
    else
    {
        send_responseline(rp, "HTTP/1.1", "200", "OK");
        send_responsehead(rp, "Server", "shouyi-lee Web Server");
        send_responsehead(rp, "Connection", "keep-alive");
        send_responsehead(rp, "Accept-Ranges", "bytes");
        send_responsehead(rp, "Content-length", filelenth_b);
        send_responsehead(rp, "Content-type", filetype);
        send_responsehead(rp, NULL, NULL);
    }

    if (!strcmp(hrp->method, "HEAD"))
        return;

    int fd = open(hrp->filename, O_RDONLY);
    if (fd < 0) return;

    if (send_partial)
        lseek(fd, range_start, SEEK_SET);

    off_t offset = 0;
    ssize_t remaining = (ssize_t)content_length;
    while (remaining > 0)
    {
        ssize_t sent = sendfile(rp->rio_fd, fd, &offset, remaining);
        if (sent < 0)
        {
            if (errno == EINTR) continue;
            break;
        }
        remaining -= sent;
    }
    close(fd);
}