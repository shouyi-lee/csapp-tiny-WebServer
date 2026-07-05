#include "time_stamp.h"
#include <time.h>

char *time_stamp(char *buf)
{
    if (!buf)
        return NULL;

    /* 获取当前时间 (秒级精度) */
    time_t now;
    if (time(&now) == (time_t)-1)
        return NULL;

    /* 转为本地时间 (localtime_r 为线程安全版本) */
    struct tm local;
    if (!localtime_r(&now, &local))
        return NULL;

    /* 格式化为 "YYYY-MM-DD HH:MM:SS" */
    if (strftime(buf, TIMESTAMP_LEN, "%F %T", &local) == 0)
        return NULL;

    return buf;
}

char *time_stamp_s(char *buf, size_t len)
{
    if (len < TIMESTAMP_LEN)
        return NULL;
    return time_stamp(buf);
}
