# 《深入理解计算机原理》示例网页服务器的实现

基于《深入理解计算机系统》(CS:APP) 示例的预线程化事件驱动 Web 服务器，采用 epoll + 线程池 + 信号量 的高并发架构。

## 编译与运行

### 编译

```bash
# Release 构建 (默认 -O2 优化)
make

# Debug 构建 (带符号表)
make DEBUG=t

# 清理 & 重新构建
make rebuild

# 清理产物
make clean
```
### 运行

```bash
# 前台运行 (默认端口 1145)
make run

# 指定端口后台运行
make run RUN_PORT=8080 RUN_COND='&'

# 或直接执行
./webserver 8080 &
```

## HTTP 支持

### 请求方法

| 方法 | 支持 | 说明 |
|------|------|------|
| GET | ✅ | 暂时只支持静态文件 |
| HEAD | ✅ | 返回与 GET 相同的响应头（无 body） |
| POST | ❌ | 返回 501 Not Implemented |

### 响应状态码

| 状态码 | 触发条件 |
|--------|----------|
| 200 OK | 请求成功 |
| 404 Not Found | 文件不存在 / 路径校验失败 / 权限不足 |
| 501 Not Implemented | 不支持的 HTTP 方法 |

## 日志

日志文件路径：`./log_file/server_log`，格式：

```
YYYY-MM-DD HH:MM:SS [info] METHOD /path -> 200 OK
YYYY-MM-DD HH:MM:SS [error]: 错误类型     METHOD /path
```

支持的错误日志类型：`invalid_request`、`unsuported method`、`invalid request head`、`invalid url`、`send error`