#ifndef LYNX_NET_TCP_SERVER_H
#define LYNX_NET_TCP_SERVER_H

#include "lynx/base/noncopyable.h"
#include "lynx/net/callbacks.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_connection.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>

namespace lynx {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer : Noncopyable {
public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop *loop, const InetAddress &listenAddr,
            const std::string &name, Option option = kNoReusePort);
  ~TcpServer();

  const std::string &ipPort() const { return ip_port_; }
  const std::string &name() const { return name_; }
  EventLoop *getLoop() const { return loop_; }

  void setThreadNum(int numThreads);

  void setThreadInitCallback(const ThreadInitCallback &cb) {
    thread_init_callback_ = cb;
  }
  std::shared_ptr<EventLoopThreadPool> threadPool() { return thread_pool_; }

  void start();

  void setConnectionCallback(const ConnectionCallback &cb) {
    connection_callback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    write_complete_callback_ = cb;
  }

private:
  void newConnection(int sockfd, const InetAddress &peerAddr);
  void removeConnection(const TcpConnectionPtr &conn);
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

  EventLoop *loop_;
  const std::string ip_port_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> thread_pool_;
  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  ThreadInitCallback thread_init_callback_;
  std::atomic_int32_t started_;
  int next_conn_id_;
  ConnectionMap connections_;
};

} // namespace lynx

#endif
