## Quick Start

```shell
./build.sh
./Webserver [port] [thread nums]
```



## 项目描述

实现了一个基于 Reactor 模型的多线程网络库，采用 One Loop Per Thread 和 Thread Pool 的编程模式，且支持解析 HTTP 头部并响应 HTTP GET 请求，同时实现了服务器定时断开空闲连接和异步日志库功能。  
## 项目内容:
- 使用 Epoll(工作模式为 LT)的 I/O 复用模型，并且结合非阻塞 I/O 实现主从 Reactor 模型;
- 利用有限状态机解析 HTTP 请求报文，高效的解析 HTTP 请求中的 URI 以及 HTTP 头部参数;
- 采用 Eventfd 便于派发事件到其他线程执行任务，优化主线程和子线程间的事件通知效率;
- 使用后端线程负责向磁盘写入前端日志实现异步日志库，且基于双缓冲机制优化写入性能;
- 采用 Timerfd 作为定时器触发描述符，基于红黑树实现管理定时器结构，高效的处理定时器触发事件。

## 代码统计

<img src="/Users/weizhihao/Library/Application Support/typora-user-images/image-20240923143017370.png" alt="image-20240923143017370" style="zoom:50%;" />