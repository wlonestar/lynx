#ifndef LYNX_NET_TCP_CONNECTION_H
#define LYNX_NET_TCP_CONNECTION_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"
#include "lynx/net/buffer.h"
#include "lynx/net/inet_address.h"

#include <functional>
#include <memory>
#include <netinet/tcp.h>

namespace lynx {

class Channel;
class EventLoop;
class Socket;

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, size_t)>;
using MessageCallback =
    std::function<void(const TcpConnectionPtr &, Buffer *, Timestamp)>;

/**
 * @class TcpConnection
 * @brief Manages a single TCP connection.
 *
 * The TcpConnection class represents a single TCP connection, providing methods
 * for reading, writing, and handling connection events. It integrates with the
 * EventLoop to handle events and callbacks efficiently.
 */
class TcpConnection : Noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  /**
   * @brief Constructs a TcpConnection with the given parameters.
   *
   * @param loop The EventLoop that manages this connection.
   * @param name The name of the connection.
   * @param sockfd The socket file descriptor.
   * @param localAddr The local address of the connection.
   * @param peerAddr The peer address of the connection.
   */
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return local_addr_; }
  const InetAddress &peerAddress() const { return peer_addr_; }
  bool connected() const { return state_ == CONNECTED; }
  bool disconnected() const { return state_ == DISCONNECTED; }

  bool getTcpInfo(struct tcp_info *) const;
  std::string getTcpInfoString() const;

  void send(const void *data, int len);
  void send(const std::string &message);
  void send(Buffer *buf);

  void shutdown();
  void forceClose();

  /**
   * @brief Sets the TCP_NODELAY option for the connection.
   *
   * @param on True to enable TCP_NODELAY, false to disable.
   */
  void setTcpNoDelay(bool on);

  /// Starts reading from the connection.
  void startRead();

  /// Stops reading from the connection.
  void stopRead();

  /**
   * @brief Checks if the connection is currently reading.
   *
   * @return True if the connection is reading, false otherwise.
   */
  bool isReading() const { return reading_; }

  void setConnectionCallback(const ConnectionCallback &cb) {
    connection_callback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    write_complete_callback_ = cb;
  }
  void setCloseCallback(const CloseCallback &cb) { close_callback_ = cb; }
  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                size_t highWaterMark) {
    high_water_mark_callback_ = cb;
    high_water_mark_ = highWaterMark;
  }

  /// Establishes the connection.
  void connectEstablished();

  /// Destroys the connection.
  void connectDestroyed();

private:
  enum StateE {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
  };

  void setState(StateE s) { state_ = s; }
  const char *stateToString() const;

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const std::string &message);
  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();
  void forceCloseInLoop();
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop *loop_;
  const std::string name_;
  StateE state_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress local_addr_;
  const InetAddress peer_addr_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  CloseCallback close_callback_;
  HighWaterMarkCallback high_water_mark_callback_;
  size_t high_water_mark_;

  Buffer input_buffer_;
  Buffer output_buffer_;
};

void defaultConnectionCallback(const TcpConnectionPtr &conn);
void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buffer,
                            Timestamp receiveTime);

} // namespace lynx

#endif
