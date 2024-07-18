#include "lynx/http/http_server.h"
#include "lynx/http/http_context.h"
#include "lynx/http/http_request.h"
#include "lynx/http/http_response.h"
#include "lynx/logger/logging.h"

#include <memory>

namespace lynx {

namespace detail {

void defaultHttpCallback(const HttpRequest & /*unused*/, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::NotFound404);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

} // namespace detail

HttpServer::HttpServer(EventLoop *loop, const InetAddress &listenAddr,
                       const std::string &name, TcpServer::Option option)
    : server_(loop, listenAddr, name, option),
      http_callback_(detail::defaultHttpCallback) {
  server_.setConnectionCallback(
      [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
  server_.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
    onMessage(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2),
              std::forward<decltype(PH3)>(PH3));
  });
}

void HttpServer::start() {
  LOG_WARN << "HttpServer[" << server_.name() << "] starts listening on "
           << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn) {
  if (conn->connected()) {
    LOG_INFO << "new Connection arrived";
  } else {
    LOG_INFO << "Connection closed";
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf,
                           Timestamp receiveTime) {
  std::unique_ptr<HttpContext> context(new HttpContext);
  context->start();

  std::string msg(buf->retrieveAllAsString());

  if (!context->parseRequest(msg.data(), msg.size())) {
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  if (context->isFinished()) {
    onRequest(conn, context->request());
    context.reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn,
                           const HttpRequest &req) {
  const std::string &connection = req.getHeader("Connection");
  bool close = connection == "close" ||
               (req.version() == 0x10 && connection != "Keep-Alive");
  HttpResponse response(close);
  http_callback_(req, &response);
  Buffer buf;
  response.appendToBuffer(&buf);
  conn->send(&buf);
  if (response.closeConnection()) {
    conn->shutdown();
  }
}

} // namespace lynx
