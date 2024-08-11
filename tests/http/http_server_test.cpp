#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/http/http_server.h"
#include "lynx/logger/async_logging.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"

#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <utility>

extern unsigned char favicon_jpg[];
extern unsigned int favicon_jpg_len;

off_t roll_size = 500 * 1000 * 1000;
lynx::AsyncLogging *g_async_log = nullptr;

void asyncOutput(const char *msg, int len) { g_async_log->append(msg, len); }

void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp);
void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp);
void handleNotFound(const lynx::HttpRequest &req, lynx::HttpResponse *resp);
void setupRoutes();

void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp);

using HttpHandler = lynx::HttpServer::HttpCallback;

inline std::map<std::pair<lynx::HttpMethod, std::string>, HttpHandler>
    g_route_table;

inline int addRoute(const std::string &method, const std::string &path,
                    HttpHandler handler) {
  g_route_table[std::make_pair(lynx::stringToHttpMethod(method), path)] =
      handler;
  return 0;
}

int main(int argc, char *argv[]) {
  char name[256] = {'\0'};
  strncpy(name, argv[0], sizeof(name) - 1);
  lynx::AsyncLogging log(::basename(name), roll_size);
  lynx::Logger::setOutput(asyncOutput);
  log.start();
  g_async_log = &log;

  int num_threads = 5;
  if (argc > 1) {
    num_threads = atoi(argv[1]);
  }
  lynx::EventLoop loop;
  lynx::HttpServer server(&loop, lynx::InetAddress(8000), "dummy");
  server.setHttpCallback(onRequest);
  server.setThreadNum(num_threads);
  LOG_INFO << "start HTTP server with " << num_threads << " threads";

  setupRoutes();

  server.start();
  loop.loop();
}

void setupRoutes() {
  addRoute("GET", "/", handleIndex);
  addRoute("GET", "/favicon.ico", handleFavicon);
}

void handleIndex(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::OK);
  resp->setContentType("text/html");
  resp->addHeader("Server", "lynx");
  std::string now = lynx::Timestamp::now().toFormattedString();
  resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " +
                now + "</body></html>");
}

void handleFavicon(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::OK);
  resp->setContentType("image/png");
  resp->setBody(
      std::string(reinterpret_cast<char *>(favicon_jpg), favicon_jpg_len));
}

void handleNotFound(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  resp->setStatusCode(lynx::HttpStatus::NOT_FOUND);
  resp->setCloseConnection(true);
}

void onRequest(const lynx::HttpRequest &req, lynx::HttpResponse *resp) {
  LOG_INFO << lynx::methodToString(req.method()) << " " << req.path();

  const std::map<std::string, std::string, lynx::CaseInsensitiveLess> &headers =
      req.headers();
  std::stringstream ss;
  for (const auto &header : headers) {
    ss << header.first << ": " << header.second << "|";
  }
  LOG_INFO << ss.str();

  auto method = req.method();
  auto &path = req.path();

  bool flag = false;
  for (auto &[pair, handler] : g_route_table) {
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
    handleNotFound(req, resp);
  }
}
