#ifndef LYNX_WEB_WEB_SERVER_H
#define LYNX_WEB_WEB_SERVER_H

#include "lynx/db/pg_connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"

#include <regex>

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

namespace lynx {

using HttpHandler = HttpServer::HttpCallback;

class WebServer {
public:
  explicit WebServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &name);

  void start() { server_.start(); }
  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  int addRoute(const std::string &method, const std::string &path,
               HttpHandler handler);

  void printRoutes();

private:
  void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp);

  HttpServer server_;
  std::map<std::pair<HttpMethod, std::string>, HttpHandler> route_table_;
};

} // namespace lynx

#endif
