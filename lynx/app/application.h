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

/**
 * @class Application
 * @brief A class for setting up a application server, including http server,
 * database connection pool.
 *
 * This class mainly contains a route table for registering http handler and
 * automatically recognizing handler by http request method and path.
 */
class Application : Noncopyable {
public:
  /**
   * @brief Constructor of @class `Application`.
   *
   * @param filename The name of the configuration file. Default is
   * "config.yml".
   *
   * @note read config yaml format file from the executable file located dir's
   * `conf/` directory.
   */
  explicit Application(const std::string &filename = "config.yml");
  ~Application();

  /**
   * @brief Start the application.
   *
   * @note If the configuration file contains `db` keys, create a
   * `ConnectionPool` object.
   */
  void start();

  /// Listening the http request.
  void listen();

  /**
   * @brief Return the reference of @class ConnectionPool.
   *
   * @note Only available if `pool_` is not null.
   */
  ConnectionPool &pool() const;

  /**
   * @brief Manually add a route to route table.
   *
   * @param method The HTTP method of the route.
   * @param path The URL path of the route.
   * @param handler The handler function for the route.
   */
  void addRoute(const std::string &method, const std::string &path,
                HttpHandler handler);

  /// Print the route table.
  void printRouteTable();

private:
  /**
   * @brief Load the configuration from a yaml file.
   *
   * @param filePath The path of the configuration file.
   */
  void loadConfig(const std::string &filePath);

  /**
   * @brief Implement the processing of matching and calling http handler, call
   * 404 Not Found handler if can not find corresponding http handler.
   *
   * @param req The HTTP request.
   * @param resp The HTTP response.
   */
  void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp);

  EventLoop *loop_;                      /// Event loop for the application.
  std::unique_ptr<HttpServer> server_;   /// HTTP server of the application.
  std::unique_ptr<ConnectionPool> pool_; /// Connection pool of the application.

  using ConfigMap = std::map<std::string, std::map<std::string, std::string>>;
  using RouteMap = std::map<std::pair<HttpMethod, std::string>, HttpHandler>;

  ConfigMap config_map_; /// The configuration map loaded from file.
  RouteMap route_table_; /// The route table for handling HTTP requests.
};

} // namespace lynx

#endif
