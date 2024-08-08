#ifndef LYNX_WEB_WEB_SERVER_H
#define LYNX_WEB_WEB_SERVER_H

#include "lynx/db/connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"

/// For favicon
extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

namespace lynx {

using HttpHandler = HttpServer::HttpCallback;

class WebServer {
public:
  explicit WebServer(EventLoop *loop,
                     const std::string &filename = "config.yml");

  void start();

  /// Check if `pool_` valid before
  ConnectionPool &pool() const;

  int addRoute(const std::string &method, const std::string &path,
               HttpHandler handler);

  void printRouteTable();

private:
  /// load yaml file with format:
  /// server:
  ///   name: WebServer
  ///   port: 8000
  ///   threads: 5
  /// db:
  ///   name: PgConnPool
  ///   host: 127.0.0.1
  ///   port: 5432
  ///   user: postgres
  ///   password: 123456
  ///   dbname: demo
  ///   min_size: 5
  ///   max_size: 10
  ///   timeout: 10
  ///   max_idle_time: 5000
  void loadConfig(const std::string &filePath);

  void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp);

  std::unique_ptr<HttpServer> server_;
  std::unique_ptr<ConnectionPool> pool_;

  std::map<std::string, std::map<std::string, std::string>> config_map_;
  std::map<std::pair<HttpMethod, std::string>, HttpHandler> route_table_;
};

} // namespace lynx

#endif
