#ifndef LYNX_HTTP_HTTP_SERVER_H
#define LYNX_HTTP_HTTP_SERVER_H

#include "lynx/net/EventLoop.h"
#include "lynx/net/InetAddress.h"
#include "lynx/net/TcpServer.h"

#include <functional>

namespace lynx {

class HttpRequest;
class HttpResponse;

/**
 * @class HttpServer
 * @brief A class that represents a HTTP server.
 *
 * It provides methods for setting up event loops, handling HTTP requests.
 */
class HttpServer {
public:
  using HttpCallbackTy = std::function<void(const HttpRequest &, HttpResponse *)>;

  /**
   * @brief Constructor for HttpServer
   *
   * @param loop Pointer to the event loop
   * @param listenAddr IP address and port number of the server to listen on
   * @param name Server name, used for logging or other purposes
   * @param option TcpServer options, specifying whether the port can be reused,
   * etc.
   */
  HttpServer(EventLoop *Loop, const InetAddress &ListenAddr,
             const std::string &Name,
             TcpServer::Option Option = TcpServer::NO_REUSE_PORT);

  EventLoop *getLoop() const { return Server.getLoop(); }

  void setHttpCallback(const HttpCallbackTy &Cb) { HttpCallback = Cb; }
  void setThreadNum(int NumThreads) { Server.setThreadNum(NumThreads); }

  void start();

private:
  /// Called when a new TCP connection is established
  void onConnection(const TcpConnectionPtr &Conn);

  /// Called when data is received on a TCP connection
  void onMessage(const TcpConnectionPtr &Conn, Buffer *Buf,
                 Timestamp ReceiveTime);

  /// Called when an HTTP request is received on a TCP connection
  void onRequest(const TcpConnectionPtr &Conn, const HttpRequest &Req);

  TcpServer Server;
  HttpCallbackTy HttpCallback;
};

} // namespace lynx

#endif
