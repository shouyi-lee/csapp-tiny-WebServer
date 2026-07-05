#ifndef TIME_STAMP_H
#define TIME_STAMP_H

#include <stddef.h>

/* 时间戳字符串最小缓冲区大小.
 * "%F %T" = "YYYY-MM-DD HH:MM:SS" = 19 字符 + '\0' = 20 字节,
 * 向上取到 32 以留余量. */
#define TIMESTAMP_LEN 32

/*
 * 将当前本地时间格式化为 "YYYY-MM-DD HH:MM:SS" 写入 buf.
 * buf 至少需要 TIMESTAMP_LEN 字节.
 * 成功返回 buf, 失败返回 NULL.
 * 线程安全.
 */
char *time_stamp(char *buf);

/*
 * 同 time_stamp, 但显式指定 buf 长度.
 * 若 len < TIMESTAMP_LEN 则返回 NULL (避免截断输出).
 */
char *time_stamp_s(char *buf, size_t len);

#endif
