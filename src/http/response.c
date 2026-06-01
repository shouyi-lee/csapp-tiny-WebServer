#include "config.h"
#include "response.h"
#include <stdio.h>
#include <string.h>
#include "rio_io.h"

typedef struct {
    char *code;
    char *desc;
} HttpStatusMap;

static const HttpStatusMap http_status_map[] = {
    {"200", "OK"},
    {"201", "Created"},
    {"204", "No Content"},
    {"301", "Moved Permanently"},
    {"302", "Found"},
    {"400", "Bad Request"},
    {"401", "Unauthorized"},
    {"403", "Forbidden"},
    {"404", "Not Found"},
    {"500", "Internal Server Error"},
    {"502", "Bad Gateway"},
    {"503", "Service Unavailable"}
};

static const char *http_status_desc(const char *code)
{
    size_t count = sizeof(http_status_map) / sizeof(http_status_map[0]);
    for (size_t i = 0; i < count; ++i) {
        if (!strcmp(code, http_status_map[i].code)) {
            return http_status_map[i].desc;
        }
    }
    return "Unknown Status";
}

ssize_t clienterror(rio_t *rp, const char *code)
{
    const char *message = http_status_desc(code);
    send_responseline(rp, "HTTP/1.1", code, message);
    send_responsehead(rp, NULL, NULL);
    return 0;
}

ssize_t send_responseline(rio_t *rp, const char *version, const char* code, const char *message)
{
    char buf[BUFLEN];
    int len = snprintf(buf, BUFLEN, "%s %s %s\r\n", version, code, message);
    
    return rio_writenb(rp, buf, len);
}

ssize_t send_responsehead(rio_t *rp, const char *head_name, const char *head_data)
{
    if (head_name == NULL)
        return rio_writenb(rp, "\r\n", 2);

    char buf[BUFLEN];
    int len = snprintf(buf, BUFLEN, "%s: %s\r\n", head_name, head_data);
    
    return rio_writenb(rp, buf, len);
}
