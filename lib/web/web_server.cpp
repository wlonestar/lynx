#include "lynx/web/web_server.h"
#include "lynx/http/http_server.h"
#include "lynx/logger/logging.h"

namespace lynx {

namespace detail {

void handleIndex(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = Timestamp::now().toFormattedString();
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

void handleNotFound(const HttpRequest &req, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::NotFound404);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

} // namespace detail

WebServer::WebServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &name)
    : server_(loop, listenAddr, name) {
  server_.setHttpCallback([this](auto &&PH1, auto &&PH2) {
    onRequest(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2));
  });
  addRoute("GET", "/", detail::handleIndex);
  addRoute("GET", "/favicon.ico", detail::handleFavicon);
}

int WebServer::addRoute(const std::string &method, const std::string &path,
                        HttpHandler handler) {
  route_table_[std::make_pair(stringToHttpMethod(method), path)] = handler;
  return 0;
}

void WebServer::printRoutes() {
  std::cout << "Route Table:\n";
  for (auto &[pair, handler] : route_table_) {
    printf("%6s - %s\n", methodToString(pair.first), pair.second.c_str());
  }
}

void WebServer::onRequest(const lynx::HttpRequest &req,
                          lynx::HttpResponse *resp) {
  std::cout << req.toString() << "\n";

  /// Searching for path with query
  std::string path = std::string(req.path());
  if (!req.query().empty()) {
    path += "?" + std::string(req.query());
  }
  LOG_DEBUG << "search for '" << path << "'";

  bool flag = false;
  auto method = req.method();
  for (auto &[pair, handler] : route_table_) {
    if (method == pair.first) {
      std::regex path_regex(pair.second);
      bool match = std::regex_match(path, path_regex);
      if (match) {
        flag = true;
        handler(req, resp);
        break;
      }
    }
  }
  if (!flag) {
    LOG_DEBUG << "not found";
    detail::handleNotFound(req, resp);
  }
}

} // namespace lynx
