# Tiny Web Server

基于《深入理解计算机系统》(CS:APP) 示例的预线程化事件驱动 Web 服务器，采用 **epoll + 线程池 + 信号量** 的高并发架构。

## 目录

- [特性](#特性)
- [架构设计](#架构设计)
- [项目结构](#项目结构)
- [编译与运行](#编译与运行)
- [HTTP 支持](#http-支持)
- [MIME 类型支持](#mime-类型支持)
- [安全机制](#安全机制)
- [配置参数](#配置参数)
- [日志](#日志)
- [测试验证](#测试验证)

## 特性

- **预线程化线程池** — 16 个工作线程 + epoll 监听线程，避免请求到来时的线程创建开销
- **epoll 事件驱动** — 使用 `epoll` 替代 `select`，突破 FD_SETSIZE 限制，O(1) 事件分发
- **连接复用 (Keep-Alive)** — 支持 HTTP/1.1 持久连接，空闲超时 5 秒自动回收
- **HTTP Range 请求** — 支持断点续传（206 Partial Content），适用于大文件/音视频流
- **URL 百分号解码** — 自动解码 `%XX` 编码的 URL
- **CGI 动态内容** — 支持 `cgi-bin` 路径的动态脚本执行
- **目录穿越防护** — 基于 `file_level` 计数的路径规范化检查，阻止 `..` 逃逸
- **结构化日志** — 线程安全的带时间戳日志，区分 info/error 级别
- **健壮 I/O (Rio)** — 带缓冲的健壮 I/O 库，处理 EINTR 信号中断、短计数

## 架构设计

```
main()
  ├── log_init()          — 日志初始化
  ├── pool_init()         — 线程池初始化 (epoll + 信号量)
  ├── serve_init()        — 16 个工作线程启动
  └── accept 循环          — 接收连接 → customer_add()

customer_listen 线程 (1个)
  └── epoll_wait() 监听所有客户端 fd
       ├── 有事件 → task_register() 将客户加入任务队列
       └── 超时   → 扫描并回收空闲超过 5s 的连接

serve 线程 (16个)
  └── task_acquire() 获取任务
       └── doit()
            ├── parse_http_request_line()   — 解析请求行
            ├── parse_http_request_head()   — 解析请求头
            ├── parse_url()                 — URL 解码、路径校验、CGI 检测
            ├── serve_static()              — 静态文件服务 (含 Range)
            └── serve_dynamic()             — CGI 动态服务
       └── task_return() — 根据 keep-alive 决定复用/关闭连接

组件依赖
  server ──► pool ──► epoll + 信号量 + 互斥锁
    │          │
    ├── request ──► rio_io (健壮缓冲 I/O)
    ├── parse_url ──► URL 解码 + 路径遍历检测
    ├── static ──► sendfile 零拷贝 + build_response
    ├── dynamic ──► fork + execve (CGI)
    ├── response ──► HTTP 状态行/头构建
    └── log ──► time_stamp + 线程安全写入
```

## 项目结构

```
.
├── Makefile                  # 构建系统
├── readme.md
├── website/                  # Web 根目录 (./website)
│   ├── index.html            # 默认首页
│   ├── common.css
│   ├── config/
│   │   ├── anime
│   │   ├── games
│   │   └── music
│   └── music/                # 音频文件 (Range 请求演示)
│       └── *.mp3 / *.m4a
└── src/
    ├── include/              # 头文件
    │   ├── config.h          # 全局配置 (_GNU_SOURCE, BUFLEN)
    │   ├── rio_io.h          # 健壮 I/O 接口
    │   ├── pool.h            # 线程池接口 (epoll + 信号量)
    │   ├── request.h         # HTTP 请求结构体
    │   ├── parse_url.h       # URL 解析声明
    │   ├── response.h        # HTTP 响应构建
    │   ├── static.h          # 静态文件服务
    │   ├── dynamic.h         # CGI 动态服务
    │   ├── server.h          # 服务初始化
    │   ├── pack_socket.h     # 套接字封装
    │   ├── log.h             # 日志接口
    │   ├── sigset.h          # 信号处理
    │   └── time_stamp.h      # 时间戳工具
    ├── server/               # 入口 & 监听
    │   ├── main.c            # 主函数 (参数解析, accept 循环)
    │   ├── server.c          # doit(), serve() 线程入口
    │   └── pack_socket.c     # getaddrinfo + bind + listen
    ├── pool/
    │   └── pool.c            # epoll 线程池实现
    ├── http/                  # HTTP 协议处理
    │   ├── request.c         # 请求行/头解析
    │   ├── parse_url.c       # URL 解码 + 路径校验 + CGI 检测
    │   ├── response.c        # 状态行/响应头构建 + 错误响应
    │   ├── static.c          # 静态文件服务 (sendfile + Range)
    │   └── dynamic.c         # CGI (fork + execve)
    ├── rio/
    │   └── rio_io.c          # 带缓冲健壮 I/O (readn/writen/readlineb)
    ├── log/
    │   └── log.c             # 线程安全结构化日志
    ├── time/
    │   └── time_stamp.c      # 本地时间戳 (localtime_r)
    └── signal/
        └── sigset.c          # SIGPIPE 忽略
```

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

编译器要求：GCC，支持 `-std=gnu11`（需 `_GNU_SOURCE` 提供 `strcasestr`、`sendfile` 等扩展）。

### 运行

```bash
# 前台运行 (默认端口 1145)
make run

# 指定端口后台运行
make run RUN_PORT=8080 RUN_COND='&'

# 或直接执行
./webserver 8080 &
```

### 目录要求

运行前需确保 `website/index.html` 存在（`make run` 会检查），日志目录 `log_file/` 自动创建。

## HTTP 支持

### 请求方法

| 方法 | 支持 | 说明 |
|------|------|------|
| GET | ✅ | 静态文件 + CGI 动态内容 |
| HEAD | ✅ | 返回与 GET 相同的响应头（无 body） |
| POST | ❌ | 返回 501 Not Implemented |

### 响应状态码

| 状态码 | 触发条件 |
|--------|----------|
| 200 OK | 请求成功 |
| 206 Partial Content | Range 请求的断点续传 |
| 404 Not Found | 文件不存在 / 路径校验失败 / 权限不足 |
| 501 Not Implemented | 不支持的 HTTP 方法 |

### HTTP Range (断点续传)

支持 `Range: bytes=N-M` 和 `Range: bytes=N-` 两种格式：

```bash
# 请求前 100 字节
curl -H "Range: bytes=0-99" http://localhost:1145/music/song.mp3

# 从第 1000 字节开始到末尾
curl -H "Range: bytes=1000-" http://localhost:1145/music/song.mp3
```

服务器返回 206 Partial Content 时包含 `Content-Range: bytes N-M/total` 响应头。

### Keep-Alive

HTTP/1.1 连接默认保持。客户端发送 `Connection: close` 时服务器关闭连接。空闲连接超过 5 秒无数据自动回收。

### CGI 动态内容

URL 包含 `cgi-bin` 的可执行文件以 CGI 方式运行：

- 查询字符串（`?` 后内容）通过 `QUERY_STRING` 环境变量传递
- 标准输出直接写入客户端 socket
- CGI 请求不保持连接（`keep-alive` 强制关闭）

## MIME 类型支持

服务器根据文件扩展名自动识别 MIME 类型，共支持 **26 种**：

| 类别 | 扩展名 |
|------|--------|
| 网页 | `.html` `.htm` `.css` `.js` `.json` |
| 图片 | `.png` `.jpg` `.jpeg` `.gif` `.ico` `.svg` |
| 文本 | `.txt` |
| 文档 | `.pdf` `.zip` |
| 音频 | `.mp3` `.wav` `.ogg` `.flac` `.aac` `.m4a` `.wma` `.aiff` `.webm` `.opus` |
| 视频 | `.mp4` |

未识别的扩展名默认使用 `application/octet-stream`。

## 安全机制

### 目录穿越防护

采用路径段逐一计数的方式验证路径合法性，而非简单的 `..` 子串匹配。以 `/` 分割 URL 路径：

- 遇到 `/..` → `file_level--`
- 遇到正常目录名 → `file_level++`
- 遇到 `/.` → `file_level` 不变

最终 `file_level <= 0` 表示路径试图逃逸到 `website/` 之上，直接拒绝并返回 404。

### 文件校验

静态文件服务在 URL 解析阶段进行三重校验：
1. `stat()` 检查文件是否存在
2. `S_ISREG()` 确保是普通文件（拒绝目录、设备文件、符号链接等）
3. `S_IRUSR` 检查文件可读权限

### 信号安全

- 忽略 `SIGPIPE`，避免写入已关闭的客户端连接时进程异常退出
- I/O 函数中 `EINTR` 自动重试，避免信号中断导致的读写失败

## 配置参数

所有配置通过 `Makefile` 变量控制：

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `RUN_PORT` | `1145` | 监听端口 |
| `RUN_COND` | `&` | 后台运行符号（前台运行设为空） |
| `DEBUG` | `f` | `t` 启用 `-g` 调试符号，`f` 启用 `-O2` 优化 |
| `CC` | `gcc` | C 编译器 |
| `TARGET` | `webserver` | 可执行文件名 |

`src/include/config.h` 中的编译期常量：

| 常量 | 值 | 说明 |
|------|------|------|
| `BUFLEN` | `4096` | 通用缓冲区大小 |
| `TASK_NUM` | `16` | 任务队列长度 |
| `CUSTOMER_NUM` | `256` | 最大并发连接数 |
| `TIME_OUT` | `1024` | epoll 超时 (ms)，同时控制空闲回收周期 |
| `SERVE_THREAD_NUM` | `16` | 工作线程数 |
| `RIO_BUFSIZE` | `4096` | Rio 内部缓冲区大小 |

空闲连接超时阈值为 **5 秒**（`pool.c` 中硬编码）。

## 日志

日志文件路径：`./log_file/server_log`，格式：

```
YYYY-MM-DD HH:MM:SS [info] METHOD /path -> 200 OK
YYYY-MM-DD HH:MM:SS [error]: 错误类型     METHOD /path
```

支持的错误日志类型：`invalid_request`、`unsuported method`、`invalid request head`、`invalid url`、`send error`。日志写入使用互斥锁保证多线程安全。

## 测试验证

```bash
# 启动服务器
make run

# 基础 GET 请求
curl -v http://localhost:1145/

# HEAD 请求
curl -I http://localhost:1145/index.html

# 404 测试
curl -v http://localhost:1145/nonexistent

# 目录穿越防护 (返回 404)
curl -v http://localhost:1145/../secret

# URL 解码
curl -v 'http://localhost:1145/%69%6e%64%65%78.html'

# Range 断点续传
curl -v -H "Range: bytes=0-99" http://localhost:1145/index.html

# 音频 Range 请求
curl -v -H "Range: bytes=0-1023" http://localhost:1145/music/song.mp3

# 并发测试 (需 ApacheBench)
ab -n 10000 -c 100 http://localhost:1145/

# 查看日志
tail -f log_file/server_log
```

## 技术要点

- **零拷贝静态文件服务**：使用 Linux `sendfile()` 系统调用，数据直接从页缓存传输到 socket，无需用户态拷贝
- **epoll 边缘触发替代**：使用 `EPOLLIN | EPOLLRDHUP` 水平触发模式，配合 `EPOLL_CLOEXEC` 避免 fd 泄漏
- **Rio 健壮 I/O**：内部 4KB 缓冲区 + `read()` 系统调用封装，提供 `readlineb()` 行读取接口，自动处理 `EINTR` 和短计数
- **线程安全日志**：基于 `pthread_mutex_t` 的临界区保护，使用 `localtime_r()` 可重入时间格式化
- **缓冲区溢出防护**：所有 `snprintf` 调用检查返回值，超长时截断并记录错误
