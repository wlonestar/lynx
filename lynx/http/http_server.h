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

class HttpServer : Noncopyable {
public:
  using HttpCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

  HttpServer(EventLoop *loop, const InetAddress &listenAddr,
             const std::string &name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  EventLoop *getLoop() const { return server_.getLoop(); }

  void setHttpCallback(const HttpCallback &cb) { http_callback_ = cb; }
  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  void start();

private:
  void onConnection(const TcpConnectionPtr &conn);
  void onMessage(const TcpConnectionPtr &conn, Buffer *buf,
                 Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr &, const HttpRequest &);

  TcpServer server_;
  HttpCallback http_callback_;
};

} // namespace lynx

#endif
