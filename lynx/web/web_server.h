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

namespace detail {

void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = lynx::Timestamp::now().toFormattedString();
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::Ok200);
  resp->setStatusMessage("OK");
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

void handleNotFound(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpResponse::NotFound404);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

} // namespace detail

/// TODO: need to be improved
class WebServer {
public:
  using HttpCallback = std::function<void(const HttpRequest &, HttpResponse *)>;
  using HttpHandler = std::function<void(const HttpRequest &, HttpResponse *)>;

  explicit WebServer(EventLoop *loop, const InetAddress &listenAddr,
                     const std::string &name)
      : server_(loop, listenAddr, name) {
    server_.setHttpCallback([this](auto &&PH1, auto &&PH2) {
      onRequest(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2));
    });
    addRoute("GET", "/", detail::handleIndex);
    addRoute("GET", "/favicon.ico", detail::handleFavicon);
  }

  void start() { server_.start(); }

  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  int addRoute(const std::string &method, const std::string &path,
               HttpHandler handler) {
    route_table_[std::make_pair(stringToHttpMethod(method), path)] = handler;
    return 0;
  }

  void printRoutes() {
    for (auto &[pair, handler] : route_table_) {
      std::cout << methodToString(pair.first) << " " << pair.second << "\n";
    }
  }

private:
  void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
    LOG_INFO << lynx::methodToString(req.method()) << " " << req.path();

    const std::map<std::string, std::string, lynx::CaseInsensitiveLess>
        &headers = req.headers();
    std::stringstream ss;
    for (const auto &header : headers) {
      ss << header.first << ": " << header.second << "|";
    }
    LOG_INFO << ss.str();

    auto method = req.method();
    auto &path = req.path();
    bool flag = false;
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
      detail::handleNotFound(req, resp);
    }
  }

  HttpServer server_;
  std::map<std::pair<HttpMethod, std::string>, HttpHandler> route_table_;
};

} // namespace lynx

#endif
