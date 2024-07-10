#ifndef LYNX_NET_CONNECTOR_H
#define LYNX_NET_CONNECTOR_H

#include "lynx/base/noncopyable.h"
#include "lynx/net/inet_address.h"

#include <functional>
#include <memory>

namespace lynx {

class Channel;
class EventLoop;

class Connector : Noncopyable, public std::enable_shared_from_this<Connector> {
public:
  using NewConnectionCallback = std::function<void(int)>;

  Connector(EventLoop *loop, const InetAddress &serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    new_connection_callback_ = cb;
  }

  void start();
  void restart();
  void stop();

  const InetAddress &serverAddress() const { return server_addr_; }

private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int K_MAX_RETRY_DELAY_MS = 30 * 1000;
  static const int K_INIT_RETRY_DELAY_MS = 500;

  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop *loop_;
  InetAddress server_addr_;
  bool connect_;
  States state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback new_connection_callback_;
  int retry_delay_ms_;
};

} // namespace lynx

#endif
