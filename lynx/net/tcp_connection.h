#ifndef LYNX_NET_TCP_CONNECTION_H
#define LYNX_NET_TCP_CONNECTION_H

#include "lynx/base/noncopyable.h"
#include "lynx/base/timestamp.h"
#include "lynx/net/buffer.h"
#include "lynx/net/callbacks.h"
#include "lynx/net/inet_address.h"

#include <boost/any.hpp>
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
  // return true if success.
  bool getTcpInfo(struct tcp_info *) const;
  std::string getTcpInfoString() const;

  // void send(string&& message); // C++11
  void send(const void *data, int len);
  void send(const std::string &message);
  // void send(Buffer&& message); // C++11
  void send(Buffer *buf); // this one will swap data
  void shutdown();        // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no
  // simultaneous calling
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  // reading or not
  void startRead();
  void stopRead();
  bool isReading() const {
    return reading_;
  }; // NOT thread safe, may race with start/stopReadInLoop

  void setContext(const boost::any &context) { context_ = context; }

  const boost::any &getContext() const { return context_; }

  boost::any *getMutableContext() { return &context_; }

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

  /// Advanced interface
  Buffer *inputBuffer() { return &input_buffer_; }

  Buffer *outputBuffer() { return &output_buffer_; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback &cb) { close_callback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished(); // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed(); // should be called only once

private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const std::string &message);
  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char *stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop *loop_;
  const std::string name_;
  StateE state_; // FIXME: use atomic variable
  bool reading_;
  // we don't expose those classes to client.
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
  Buffer output_buffer_; // FIXME: use list<Buffer> as output buffer.
  boost::any context_;
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

} // namespace lynx

#endif
