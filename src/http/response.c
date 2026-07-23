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

//可以认为在手动构建代码-注释映射表的前提下是健壮的
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
    if (send_responseline(rp, "HTTP/1.1", code, message) < 0) return -1;
    if (!strcmp("501", code))
        if (send_responsehead(rp, "Connection", "close") < 0)
            return -1;
    if (send_responsehead(rp, NULL, NULL) < 0) return -1;
    return 0;
}

ssize_t send_responseline(rio_t *rp, const char *version, const char* code, const char *message)
{
    char buf[BUFLEN];

    const char *real_message = message == NULL ? http_status_desc(code) : message;

    int len = snprintf(buf, BUFLEN, "%s %s %s\r\n", version, code, real_message);
    if (len >= BUFLEN || len < 0) return -1;
    
    return rio_writenb(rp, buf, len);
}

ssize_t send_responsehead(rio_t *rp, const char *head_name, const char *head_data)
{
    if (head_name == NULL)
        return rio_writenb(rp, "\r\n", 2);

    char buf[BUFLEN];
    int len = snprintf(buf, BUFLEN, "%s: %s\r\n", head_name, head_data);
    if (len >= BUFLEN || len < 0) return -1;
    
    return rio_writenb(rp, buf, len);
}

//只要构建sendbuf时也使用BUFLEN，是健壮的。
ssize_t build_responseline(char *sendbuf, const char *version, const char* code, const char *message)
{
    const char *real_message = message == NULL ? http_status_desc(code) : message;

    int len = snprintf(sendbuf, BUFLEN, "%s %s %s\r\n", version, code, real_message);
    if (len >= BUFLEN || len < 0) return -1;

    return len;//我们可以认为在linux上的int到ssize_t转换不会出毛病
}

//需要调用者自己保证当前offset加上写入长度不会溢出
ssize_t build_responsehead(char *sendbuf, const char *head_name, const char *head_data, size_t offset)
{

    char *buf = sendbuf + offset;
    int len;
    if (head_name == NULL) len = sprintf(buf, "\r\n");
    else len = snprintf(buf, BUFLEN, "%s: %s\r\n", head_name, head_data);
    if (len >= BUFLEN || len < 0) return -1;

    return len;
}