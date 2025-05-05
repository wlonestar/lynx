#include "lynx/http/HttpContext.h"
#include "lynx/http/HttpRequest.h"
#include "lynx/http/HttpResponse.h"
#include "lynx/http/HttpServer.h"
#include "lynx/logger/Logging.h"

#include <memory>

namespace lynx {

namespace detail {

void defaultHttpCallback(const HttpRequest & /*unused*/, HttpResponse *Resp) {
  Resp->setStatusCode(HttpStatus::NOT_FOUND);
  Resp->setCloseConnection(true);
}

} // namespace detail

HttpServer::HttpServer(EventLoop *Loop, const InetAddress &ListenAddr,
                       const std::string &Name, TcpServer::Option Option)
    : Server(Loop, ListenAddr, Name, Option),
      HttpCallback(detail::defaultHttpCallback) {
  Server.setConnectionCallback(
      [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
  Server.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
    onMessage(std::forward<decltype(PH1)>(PH1),
              std::forward<decltype(PH2)>(PH2),
              std::forward<decltype(PH3)>(PH3));
  });
}

void HttpServer::start() {
  LOG_WARN << "HttpServer[" << Server.name() << "] starts listening on "
           << Server.ipPort();
  Server.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &Conn) {
  if (Conn->connected()) {
    LOG_INFO << "new Connection arrived";
  } else {
    LOG_INFO << "Connection closed";
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &Conn, Buffer *Buf,
                           Timestamp ReceiveTime) {
  std::unique_ptr<HttpContext> Context(new HttpContext);
  Context->start();

  std::string Msg(Buf->retrieveAllAsString());

  if (!Context->parseRequest(Msg.data(), Msg.size())) {
    Conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    Conn->shutdown();
  }

  if (Context->isFinished()) {
    onRequest(Conn, Context->request());
    Context.reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr &Conn,
                           const HttpRequest &Req) {
  const std::string &Connection = Req.getHeader("Connection");
  bool Close = Connection == "close" ||
               (Req.version() == 0x10 && Connection != "Keep-Alive");
  HttpResponse Response(Close);
  HttpCallback(Req, &Response);
  Buffer Buf;
  Response.appendToBuffer(&Buf);
  Conn->send(&Buf);
  if (Response.closeConnection())
    Conn->shutdown();
}

} // namespace lynx
