#ifndef LYNX_HTTP_HTTP_SERVER_H
#define LYNX_HTTP_HTTP_SERVER_H

#include "lynx/base/noncopyable.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_server.h"

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
class HttpServer : Noncopyable {
public:
  using HttpCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

  /**
   * @brief Constructor for HttpServer
   *
   * @param loop Pointer to the event loop
   * @param listenAddr IP address and port number of the server to listen on
   * @param name Server name, used for logging or other purposes
   * @param option TcpServer options, specifying whether the port can be reused,
   * etc.
   */
  HttpServer(EventLoop *loop, const InetAddress &listenAddr,
             const std::string &name,
             TcpServer::Option option = TcpServer::NO_REUSE_PORT);

  EventLoop *getLoop() const { return server_.getLoop(); }

  void setHttpCallback(const HttpCallback &cb) { http_callback_ = cb; }
  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  void start();

private:
  /// Called when a new TCP connection is established
  void onConnection(const TcpConnectionPtr &conn);

  /// Called when data is received on a TCP connection
  void onMessage(const TcpConnectionPtr &conn, Buffer *buf,
                 Timestamp receiveTime);

  /// Called when an HTTP request is received on a TCP connection
  void onRequest(const TcpConnectionPtr &conn, const HttpRequest &req);

  TcpServer server_;
  HttpCallback http_callback_;
};

} // namespace lynx

#endif
