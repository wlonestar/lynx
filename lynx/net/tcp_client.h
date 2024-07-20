#ifndef LYNX_NET_TCP_CLIENT_H
#define LYNX_NET_TCP_CLIENT_H

#include "lynx/net/tcp_connection.h"

#include <mutex>

namespace lynx {

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : Noncopyable {
public:
  TcpClient(EventLoop *loop, const InetAddress &serverAddr,
            const std::string &name);
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

  EventLoop *getLoop() const { return loop_; }
  bool retry() const { return retry_; }
  void enableRetry() { retry_ = true; }

  const std::string &name() const { return name_; }

  void setConnectionCallback(ConnectionCallback cb) {
    connection_callback_ = std::move(cb);
  }
  void setMessageCallback(MessageCallback cb) {
    message_callback_ = std::move(cb);
  }
  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    write_complete_callback_ = std::move(cb);
  }

private:
  void newConnection(int sockfd);
  void removeConnection(const TcpConnectionPtr &conn);

  EventLoop *loop_;
  ConnectorPtr connector_;
  const std::string name_;
  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  bool retry_;
  bool connect_;
  int next_conn_id_;
  mutable std::mutex mutex_;
  TcpConnectionPtr connection_;
};

} // namespace lynx

#endif
