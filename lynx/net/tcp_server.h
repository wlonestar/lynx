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

  // TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop *loop, const InetAddress &listenAddr,
            const std::string &nameArg, Option option = kNoReusePort);
  ~TcpServer(); // force out-line dtor, for std::unique_ptr members.

  const std::string &ipPort() const { return ip_port_; }
  const std::string &name() const { return name_; }
  EventLoop *getLoop() const { return loop_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback &cb) {
    thread_init_callback_ = cb;
  }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool() { return thread_pool_; }

  /// Starts the server if it's not listening.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback &cb) {
    connection_callback_ = cb;
  }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    write_complete_callback_ = cb;
  }

private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress &peerAddr);
  /// Thread safe.
  void removeConnection(const TcpConnectionPtr &conn);
  /// Not thread safe, but in loop
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

  EventLoop *loop_; // the acceptor loop
  const std::string ip_port_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> thread_pool_;
  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  ThreadInitCallback thread_init_callback_;
  std::atomic_int32_t started_;
  // always in loop thread
  int next_conn_id_;
  ConnectionMap connections_;
};

} // namespace lynx

#endif
