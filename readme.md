# 《深入理解计算机系统》示例 Web 服务器的实现

基于《深入理解计算机系统》(CS:APP) 示例实现的预线程化 Web
服务器，使用 epoll、线程池和信号量处理并发连接。

## 编译

```bash
# Release 构建（默认使用 -O2）
make

# Debug 构建（包含调试符号）
make DEBUG=t

# 清理并重新构建
make rebuild

# 清理构建产物
make clean
```

## 配置

服务器启动时从配置文件读取端口、日志路径、静态资源路径、默认首页和
真实客户端 IP 请求头。`make run` 默认读取仓库中的 `server.conf`；推荐的
配置内容和格式如下：

```text
port: 1145
log_url: log_file/server_log
source_url: website
root_source: /index.html
real_ip_head: CF-Connecting-IP
```

配置项说明：

| 配置项 | 说明 |
|---|---|
| `port` | 监听端口，必须是数字端口 |
| `log_url` | 日志文件路径；文件不存在时会创建，但父目录必须已经存在 |
| `source_url` | 静态资源根目录 |
| `root_source` | 请求 `/` 时使用的默认资源 URL，应以 `/` 开头 |
| `real_ip_head` | 用于取得真实客户端 IP 的 HTTP 请求头名称，匹配时不区分大小写 |

当前配置解析器有以下限制：

- 五个配置项都必须提供，顺序不限。
- 每行格式为 `名称: 值`；值中不能包含空格或制表符。
- 不支持空行、注释或未知配置项。
- 相对路径相对于服务器进程的当前工作目录，而不是配置文件所在目录。

生产部署时建议为 `log_url` 和 `source_url` 使用绝对路径，避免启动目录改变
后访问到错误位置。例如：

```text
port: 1145
log_url: /var/log/tiny-server/server.log
source_url: /srv/tiny-server/www
root_source: /index.html
real_ip_head: X-Real-IP
```

服务进程必须对静态资源具有读取权限，并对日志文件及其父目录具有写入权限。

## 运行

使用默认的 `server.conf`：

```bash
# 默认配置将日志写入 log_file/server_log
mkdir -p log_file
make run
```

指定其他配置文件：

```bash
make run CONFIG=/etc/tiny-server/server.conf
```

也可以直接执行：

```bash
./webserver server.conf
./webserver /etc/tiny-server/server.conf
```

服务器以前台方式运行。使用 systemd 托管时，应让 `ExecStart` 直接执行
`webserver`，不要在命令末尾添加 `&`。当配置中的资源和日志路径都是绝对路径
时，不需要依赖 `WorkingDirectory`：

```ini
[Unit]
Description=Tiny HTTP Server
After=network.target

[Service]
Type=exec
ExecStart=/usr/local/bin/webserver /etc/tiny-server/server.conf
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

## 真实客户端 IP 请求头

服务器从 `real_ip_head` 配置指定的 HTTP 请求头中读取客户端 IP，并将其写入
访问日志和错误日志。修改该配置后只需重启服务器，不需要重新编译。

Cloudflare 通常使用 `CF-Connecting-IP`；Nginx 等反向代理可以配置为传递
`X-Real-IP`。建议只使用包含单个 IP 的请求头；`X-Forwarded-For` 可能包含
多级代理地址，服务器目前不会解析或校验该地址链。

如果请求中没有指定的请求头，日志中的来源 IP 将记为 `unknown ip`。服务器会
直接信任该请求头，因此部署时应阻止客户端绕过可信反向代理直接访问服务器，
并让代理覆盖外部传入的同名请求头。该 IP 目前只用于日志，不参与访问控制。

## HTTP 支持

### 请求方法

| 方法 | 支持 | 说明 |
|---|---|---|
| GET | ✅ | 提供静态文件 |
| HEAD | ✅ | 返回与 GET 相同的响应头，但不发送响应体 |
| POST | ❌ | 返回 501 |

### 响应状态码

| 状态码 | 触发条件 |
|---|---|
| 200 OK | 请求成功 |
| 404 Not Found | 文件不存在、路径校验失败或权限不足 |
| 501 Not Implemented | 请求方法尚未实现 |

## 日志

日志文件由 `log_url` 配置指定，格式如下：

```text
YYYY-MM-DD HH:MM:SS [info] SOURCE_IP   METHOD /path -> 200 OK
YYYY-MM-DD HH:MM:SS [error] SOURCE_IP   错误类型   METHOD /path
```

当前错误类型包括：

- `invalid request`
- `unsuported method`
- `invalid request head`
- `invalid url`
- `send error`
