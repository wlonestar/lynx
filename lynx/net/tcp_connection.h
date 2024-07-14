#ifndef LYNX_NET_TCP_CONNECTION_H
#define LYNX_NET_TCP_CONNECTION_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"
#include "lynx/net/buffer.h"
#include "lynx/net/callbacks.h"
#include "lynx/net/inet_address.h"

#include <functional>
#include <memory>

struct tcp_info;

namespace lynx {

class Channel;
class EventLoop;
class Socket;

class TcpConnection : Noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return local_addr_; }
  const InetAddress &peerAddress() const { return peer_addr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  bool getTcpInfo(struct tcp_info *) const;
  std::string getTcpInfoString() const;

  void send(const void *data, int len);
  void send(const std::string &message);
  void send(Buffer *buf);
  void shutdown();
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  void startRead();
  void stopRead();
  bool isReading() const { return reading_; };

  void setConnectionCallback(const ConnectionCallback &cb) {
    connection_callback_ = cb;
  }
  void setMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    write_complete_callback_ = cb;
  }
  void setHighWaterMarkCallback(const HighWaterMarkCallback &cb,
                                size_t highWaterMark) {
    high_water_mark_callback_ = cb;
    high_water_mark_ = highWaterMark;
  }

  Buffer *inputBuffer() { return &input_buffer_; }
  Buffer *outputBuffer() { return &output_buffer_; }

  void setCloseCallback(const CloseCallback &cb) { close_callback_ = cb; }

  void connectEstablished();
  void connectDestroyed();

private:
  enum StateE {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisconnecting,
  };

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const std::string &message);
  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char *stateToString() const;
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
  HighWaterMarkCallback high_water_mark_callback_;
  CloseCallback close_callback_;
  size_t high_water_mark_;

  Buffer input_buffer_;
  Buffer output_buffer_;
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

} // namespace lynx

#endif
