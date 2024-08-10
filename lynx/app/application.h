#ifndef LYNX_APP_APPLICATION_H
#define LYNX_APP_APPLICATION_H

#include "lynx/db/connection_pool.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"

namespace lynx {

using HttpHandler = HttpServer::HttpCallback;

/// @class Application
/// @brief A class for setting up a application server, including http server,
/// database connection pool.
///
/// This class mainly contains a route table for registering http handler and
/// automatically recognizing handler by http request method and path.
class Application : Noncopyable {
public:
  /// Constructor of @class `Application`.
  ///
  /// @note read config yaml format file from the executable file located dir's
  /// `conf/` directory.
  explicit Application(const std::string &filename = "config.yml");

  /// Start the application.
  ///
  /// @note Only create `ConnectionPool` object if config file contains `db`
  /// keys.
  void start();

  /// Listening the http request.
  void listen();

  /// Return the reference of @class ConnectionPool.
  ///
  /// @note Only available if `pool_` is not null.
  ConnectionPool &pool() const;

  /// Manually add a route to route table.
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

  /// Implement the processing of matching and calling http handler, call 404
  /// Not Found handler if can not find corresponding http handler.
  void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp);

  EventLoop *loop_;
  std::unique_ptr<HttpServer> server_;
  std::unique_ptr<ConnectionPool> pool_;

  std::map<std::string, std::map<std::string, std::string>> config_map_;
  std::map<std::pair<HttpMethod, std::string>, HttpHandler> route_table_;
};

} // namespace lynx

#endif
