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

### 真实客户端 IP 请求头

服务器会从 `REAL_IP_HEAD` 指定的 HTTP 请求头中读取客户端 IP，并将其写入访问日志和错误日志。默认请求头为 `CF-Connecting-IP`，适用于 Cloudflare 代理场景；请求头名匹配不区分大小写。

如果反向代理使用其他请求头，可在编译时覆盖：

```bash
# 例如 Nginx 传递 X-Real-IP
make rebuild REAL_IP_HEAD=X-Real-IP
```

`REAL_IP_HEAD` 会在编译时写入程序，修改后需要使用 `make rebuild` 重新编译。建议配置为只包含单个 IP 的请求头；`X-Forwarded-For` 可能包含多级代理地址，服务器不会解析或校验该地址链。

如果请求中没有该请求头，日志中的来源 IP 将记为 `unknown ip`。该请求头由客户端提供，服务器会直接信任其值；部署时应限制客户端绕过可信反向代理直接访问服务器，并由代理覆盖外部传入的同名请求头。此 IP 目前仅用于日志，不参与访问控制。

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
YYYY-MM-DD HH:MM:SS [info] SOURCE_IP   METHOD /path -> 200 OK
YYYY-MM-DD HH:MM:SS [error] SOURCE_IP   错误类型   METHOD /path
```

支持的错误日志类型：`invalid request`、`unsuported method`、`invalid request head`、`invalid url`、`send error`
