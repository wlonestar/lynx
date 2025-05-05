#ifndef LYNX_NET_ACCEPTOR_H
#define LYNX_NET_ACCEPTOR_H

#include "lynx/net/Channel.h"
#include "lynx/net/Socket.h"

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
class Acceptor {
public:
  using NewConnectionCallback = std::function<void(int, const InetAddress &)>;

  /**
   * @brief Constructs an Acceptor with the given parameters.
   *
   * @param loop The EventLoop that manages this Acceptor.
   * @param listenAddr The address to listen on.
   * @param reuseport Whether to enable port reuse.
   */
  Acceptor(EventLoop *Loop, const InetAddress &ListenAddr, bool Reuseport);
  ~Acceptor();

  /// Starts listening for incoming connections.
  void listen();

  bool listening() const { return Listening; }

  void setNewConnectionCallback(const NewConnectionCallback &Cb) {
    NewConnectionCb = Cb;
  }

private:
  /// Handles incoming connections.
  void handleRead();

  EventLoop *Loop;
  Socket AcceptSocket;
  Channel AcceptChannel;
  NewConnectionCallback NewConnectionCb;
  bool Listening;
  int IdleFd;
};

} // namespace lynx

#endif
