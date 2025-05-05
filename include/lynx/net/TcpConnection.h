#ifndef LYNX_NET_TCP_CONNECTION_H
#define LYNX_NET_TCP_CONNECTION_H

#include "lynx/base/Timestamp.h"
#include "lynx/net/Buffer.h"
#include "lynx/net/InetAddress.h"

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
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
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
  TcpConnection(EventLoop *Loop, const std::string &Name, int Sockfd,
                const InetAddress &LocalAddr, const InetAddress &PeerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return Loop; }
  const std::string &name() const { return Name; }
  const InetAddress &localAddress() const { return LocalAddr; }
  const InetAddress &peerAddress() const { return PeerAddr; }
  bool connected() const { return State == CONNECTED; }
  bool disconnected() const { return State == DISCONNECTED; }

  bool getTcpInfo(struct tcp_info *) const;
  std::string getTcpInfoString() const;

  void send(const void *Data, int Len);
  void send(const std::string &Message);
  void send(Buffer *Buf);

  void shutdown();
  void forceClose();

  /**
   * @brief Sets the TCP_NODELAY option for the connection.
   *
   * @param on True to enable TCP_NODELAY, false to disable.
   */
  void setTcpNoDelay(bool On);

  /// Starts reading from the connection.
  void startRead();

  /// Stops reading from the connection.
  void stopRead();

  /**
   * @brief Checks if the connection is currently reading.
   *
   * @return True if the connection is reading, false otherwise.
   */
  bool isReading() const { return Reading; }

  void setConnectionCallback(const ConnectionCallback &Cb) {
    ConnectionCallback = Cb;
  }
  void setMessageCallback(const MessageCallback &Cb) { MessageCallback = Cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &Cb) {
    WriteCompleteCallback = Cb;
  }
  void setCloseCallback(const CloseCallback &Cb) { CloseCallback = Cb; }
  void setHighWaterMarkCallback(const HighWaterMarkCallback &Cb,
                                size_t HighWMark) {
    HighWaterMarkCallback = Cb;
    HighWaterMark = HighWMark;
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

  void setState(StateE S) { State = S; }
  const char *stateToString() const;

  void handleRead(Timestamp ReceiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const std::string &Message);
  void sendInLoop(const void *Data, size_t Len);
  void shutdownInLoop();
  void forceCloseInLoop();
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop *Loop;
  const std::string Name;
  StateE State;
  bool Reading;

  std::unique_ptr<Socket> Sockt;
  std::unique_ptr<Channel> Chann;

  const InetAddress LocalAddr;
  const InetAddress PeerAddr;

  ConnectionCallback ConnectionCallback;
  MessageCallback MessageCallback;
  WriteCompleteCallback WriteCompleteCallback;
  CloseCallback CloseCallback;
  HighWaterMarkCallback HighWaterMarkCallback;
  size_t HighWaterMark;

  Buffer InputBuffer;
  Buffer OutputBuffer;
};

void defaultConnectionCallback(const TcpConnectionPtr &Conn);
void defaultMessageCallback(const TcpConnectionPtr &Conn, Buffer *Buffer,
                            Timestamp ReceiveTime);

} // namespace lynx

#endif
