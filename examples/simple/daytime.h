#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/tcp_server.h"

class DaytimeServer {
public:
  DaytimeServer(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr)
      : server_(loop, listenAddr, "DaytimeServer") {
    server_.setConnectionCallback(
        [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    server_.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
  }

  void start() { server_.start(); }

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_INFO << "DaytimeServer - " << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected()) {
      conn->send(lynx::Timestamp::now().toFormattedString() + '\n');
      conn->shutdown();
    }
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    std::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
             << " bytes received at " << time.toString();
  }

  lynx::TcpServer server_;
};
