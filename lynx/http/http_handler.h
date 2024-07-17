#ifndef LYNX_HTTP_HTTP_HANDLER_H
#define LYNX_HTTP_HTTP_HANDLER_H

#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"

#include <functional>

namespace lynx {

using HttpHandler = std::function<void(const HttpRequest &, HttpResponse *)>;

inline std::map<std::pair<HttpMethod, std::string>, HttpHandler> g_route_table;

inline int addRoute(const std::string &method, const std::string &path,
                    HttpHandler handler) {
  g_route_table[std::make_pair(stringToHttpMethod(method), path)] = handler;
  return 0;
}

#define REGISTER_HANDLER(controller, method, path, name, handler)              \
  void name(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {          \
    (controller)->handler(req, resp);                                          \
  }                                                                            \
  inline auto name##Handler = lynx::addRoute(method, path, name);

} // namespace lynx

#endif
