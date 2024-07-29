#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_server.h"

class ChargenServer {
public:
  ChargenServer(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr,
                bool print = false)
      : server_(loop, listenAddr, "ChargenServer"),
        start_time_(lynx::Timestamp::now()) {
    server_.setConnectionCallback(
        [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    server_.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
    server_.setWriteCompleteCallback([this](auto &&PH1) {
      onWriteComplete(std::forward<decltype(PH1)>(PH1));
    });
    if (print) {
      loop->runEvery(3.0, [this] { printThroughput(); });
    }

    std::string line;
    for (int i = 33; i < 127; ++i) {
      line.push_back(char(i));
    }
    line += line;

    for (size_t i = 0; i < 127 - 33; ++i) {
      message_ += line.substr(i, 72) + '\n';
    }
  }

  void start() { server_.start(); }

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_INFO << "ChargenServer - " << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
      conn->setTcpNoDelay(true);
      conn->send(message_);
    }
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    std::string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
             << " bytes received at " << time.toFormattedString();
  }

  void onWriteComplete(const lynx::TcpConnectionPtr &conn) {
    transferred_ += message_.size();
    conn->send(message_);
  }

  void printThroughput() {
    lynx::Timestamp end_time = lynx::Timestamp::now();
    double time = lynx::timeDiff(end_time, start_time_);
    printf("%4.3f MiB/s\n",
           static_cast<double>(transferred_) / time / 1024 / 1024);
    transferred_ = 0;
    start_time_ = end_time;
  }

  lynx::TcpServer server_;

  std::string message_;
  int64_t transferred_{};
  lynx::Timestamp start_time_;
};
