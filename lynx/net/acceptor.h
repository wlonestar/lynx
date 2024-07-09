#ifndef LYNX_NET_ACCEPTOR_H
#define LYNX_NET_ACCEPTOR_H

#include "lynx/base/noncopyable.h"
#include "lynx/net/channel.h"
#include "lynx/net/socket.h"

#include <cerrno>
#include <fcntl.h>
#include <functional>
#include <unistd.h>

namespace lynx {

class EventLoop;
class InetAddress;

class Acceptor : Noncopyable {
public:
  using NewConnectionCallback = std::function<void(int, const InetAddress &)>;

  Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    new_connection_callback_ = cb;
  }

  void listen();

  bool listening() const { return listening_; }

private:
  void handleRead();

  EventLoop *loop_;
  Socket accept_socket_;
  Channel accept_channel_;
  NewConnectionCallback new_connection_callback_;
  bool listening_;
  int idle_fd_;
};

} // namespace lynx

#endif
