# 《深入理解计算机原理》示例网页服务器的实现
1. 默认路径
    - 默认GET返回文件website/index.html
    - 默认日志文件log_file/server_log
2. 编译选项
    - `RUN_COND`，为`&`则后台运行
    - `RUN_PORT`，指定运行端口
---
**服务器工作原理**
- 预线程化的连接池+任务池的事件驱动服务器