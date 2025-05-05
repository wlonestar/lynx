#ifndef LYNX_NET_TCP_SERVER_H
#define LYNX_NET_TCP_SERVER_H

#include "lynx/net/InetAddress.h"
#include "lynx/net/TcpConnection.h"

#include <atomic>
#include <map>

namespace lynx {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

/**
 * @class TcpServer
 * @brief Manages a TCP server.
 *
 * The TcpServer class is responsible for managing incoming connections,
 * distributing them to threads, and handling connection events.
 */
class TcpServer {
public:
  using ThreadInitCallbackTy = std::function<void(EventLoop *)>;

  enum Option {
    NO_REUSE_PORT,
    REUSE_PORT,
  };

  /**
   * @brief Constructs a TcpServer with the given parameters.
   *
   * @param loop The EventLoop that manages this server.
   * @param listenAddr The address to listen on.
   * @param name The name of the server.
   * @param option Option for socket reuse.
   */
  TcpServer(EventLoop *Loop, const InetAddress &ListenAddr,
            const std::string &Name, Option Option = NO_REUSE_PORT);
  ~TcpServer();

  void start();

  void setThreadNum(int NumThreads);

  EventLoop *getLoop() const { return Loop; }

  const std::string &ipPort() const { return IpPort; }
  const std::string &name() const { return Name; }

  void setThreadInitCallback(const ThreadInitCallbackTy &Cb) {
    ThreadInitCallback = Cb;
  }
  void setConnectionCallback(const ConnectionCallback &Cb) {
    ConnectionCallback = Cb;
  }
  void setMessageCallback(const MessageCallback &Cb) { MessageCallback = Cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &Cb) {
    WriteCompleteCallback = Cb;
  }

private:
  void newConnection(int Sockfd, const InetAddress &PeerAddr);
  void removeConnection(const TcpConnectionPtr &Conn);
  void removeConnectionInLoop(const TcpConnectionPtr &Conn);

  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

  EventLoop *Loop;
  const std::string IpPort;
  const std::string Name;
  std::unique_ptr<Acceptor> Acceptr;

  std::shared_ptr<EventLoopThreadPool> ThreadPool;

  ConnectionCallback ConnectionCallback;
  MessageCallback MessageCallback;
  WriteCompleteCallback WriteCompleteCallback;

  ThreadInitCallbackTy ThreadInitCallback;
  std::atomic_int32_t Started;

  int NextConnId;
  ConnectionMap Connections;
};

} // namespace lynx

#endif
