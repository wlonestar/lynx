#ifndef LYNX_APP_APPLICATION_H
#define LYNX_APP_APPLICATION_H

#include "lynx/db/ConnectionPool.h"
#include "lynx/http/HttpRequest.h"
#include "lynx/http/HttpResponse.h"
#include "lynx/http/HttpServer.h"
#include "lynx/net/EventLoop.h"
#include "lynx/net/InetAddress.h"

namespace lynx {

using HttpHandler = std::function<void(const HttpRequest &, HttpResponse *)>;

/**
 * @class Application
 * @brief A class for setting up a application server, including http server,
 * database connection pool.
 *
 * This class mainly contains a route table for registering http handler and
 * automatically recognizing handler by http request method and path.
 */
class Application {
public:
  /**
   * @brief Constructor for the Application class.
   *
   * @param filename The name of the configuration file. If empty, a default
   * configuration is used.
   */
  explicit Application(const std::string &Filename = "");
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
  void addRoute(const std::string &Method, const std::string &Path,
                HttpHandler Handler);

  /// Print the route table.
  void printRouteTable();

private:
  /**
   * @brief Load the configuration from a yaml file.
   *
   * @param filePath The path of the configuration file.
   */
  void loadConfig(const std::string &FilePath);

  /**
   * @brief Implement the processing of matching and calling http handler, call
   * 404 Not Found handler if can not find corresponding http handler.
   *
   * @param req The HTTP request.
   * @param resp The HTTP response.
   */
  void onRequest(const lynx::HttpRequest &Req, lynx::HttpResponse *Resp);

  EventLoop *Loop;                      /// Event loop for the application.
  std::unique_ptr<HttpServer> Server;   /// HTTP server of the application.
  std::unique_ptr<ConnectionPool> Pool; /// Connection pool of the application.

  using ConfigMapTy = std::map<std::string, std::map<std::string, std::string>>;
  using RouteMap = std::map<std::pair<HttpMethod, std::string>, HttpHandler>;

  ConfigMapTy ConfigMap; /// The configuration map loaded from file.
  RouteMap RouteTable; /// The route table for handling HTTP requests.
};

} // namespace lynx

#endif
