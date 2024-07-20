# 网络库

基于 Reactor 模式的网络库，核心是个事件循环 EventLoop，用于响应计时器和 IO 事件。

lynx 采用基于对象（object-based）而非面向对象（object-oriented）的设计风格，事件回调接口多以 std::function + lambda 表达。

通过使用前向声明（forward declaration）简化了头文件之间的依赖关系，避免将内部类暴露给用户。

## 公开接口

- Buffer: 数据的读写通过 buffer 进行。用户代码不需要调用 read/write，只需要处理收到的数据和准备好要发送的数据。
- InetAddress: 封装 IPv4/IPv6 地址，，不能解析域名，只认 IP 地址。
- EventLoop: 事件循环（反应器 Reactor），每个线程只能有一个 EventLoop 实体，负责 IO 和定时器事件的分派。用 eventfd 来异步唤醒，有别于传统的用一对 pipe 的办法，用 TimerQueue 作为计时器管理，用 Epoller 作为 IO 多路复用。
- EventLoopThread: 启动一个线程，在其中运行 EventLoop::loop()。
- TcpConnection: 整个网络库的核心，封装一次 TCP 连接，不能发起连接。
- TcpClient: 用于编写网络客户端，能发起连接，有重试功能。
- TcpServer: 用于编写网络服务器，接受客户的连接。

TcpConnection 的生命期依靠 shared_ptr 管理（用户和库共同控制）， Buffer 的生命期由 TcpConnection 控制。其余类的生命期由用户控制。

## 内部实现

- Channel: selectable IO channel，负责注册与响应 IO 事件，不拥有 file descriptor。
- Socket: RAIIhandle，封装一个 file descriptot，并在析构时关闭 fd。
- SocketsOps: 封装各种 socket 系统调用。
- Epoller: 封装 epoll IO 多路复用后端。
- Connector: 用于发起 TCP 连接。
- Acceptor：用于接受 TCP 连接。
- TimerQueue: 用 timerfd 实现定时。
- EventLoopThreadPool: 用于创建 IO 线程池，把 TcpConnection 分派到某个 EventLoop 线程上。

## 线程模型

one loop per thread + thread pool 模型。每个线程最多有一个 EventLoop，每个 TcpConnection 必须归某个 EventLoop 管理，所有的 IO 会转移到这个线程——一个 fd 只能由一个线程读写。

TcpServer 支持多线程，有两种模式：

- 单线程， accept() 与 TcpConnection 用同一个线程做 IO。
- 多线程， accept() 与 EventLoop 在同一个线程，另外创建一个 EventLoopThreadPool，新到的连接会按 round-robin 方式分配到线程池中。

## TCP 网络编程

基于事件的非阻塞网络编程是编写高性能并发网络服务程序的主流模式。

```
主动调用 recv 来接收数据，
主动调用 accept 来接受新连接，
主动调用 send 来发送数据。

-->

注册一个收数据的回调，网络库收到数据会调用我，直接把数据提供给我，供我消费。
注册一个接受连接的回调，网络库接受了新连接会回调我，直接把新的连接对象传给我，供我使用。
需要发送数据的时候，只管往连接中写，网络库会负责无阻塞地发送。
```
