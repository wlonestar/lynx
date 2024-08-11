#ifndef LYNX_NET_ACCEPTOR_H
#define LYNX_NET_ACCEPTOR_H

#include "lynx/net/channel.h"
#include "lynx/net/socket.h"

namespace lynx {

class EventLoop;
class InetAddress;

/**
 * @class Acceptor
 * @brief Manages accepting incoming TCP connections.
 *
 * The Acceptor class is responsible for listening for incoming TCP connections
 * on a specified address and port, and notifying the server when a new
 * connection is established.
 */
class Acceptor : Noncopyable {
public:
  using NewConnectionCallback = std::function<void(int, const InetAddress &)>;

  /**
   * @brief Constructs an Acceptor with the given parameters.
   * @param loop The EventLoop that manages this Acceptor.
   * @param listenAddr The address to listen on.
   * @param reuseport Whether to enable port reuse.
   */
  Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
  ~Acceptor();

  /**
   * @brief Starts listening for incoming connections.
   */
  void listen();

  bool listening() const { return listening_; }

  void setNewConnectionCallback(const NewConnectionCallback &cb) {
    new_connection_callback_ = cb;
  }

private:
  /// Handles incoming connections.
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
