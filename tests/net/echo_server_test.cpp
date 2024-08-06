#include "lynx/base/thread.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_server.h"

int num_threads = 0;

class EchoServer {
public:
  EchoServer(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr)
      : loop_(loop), server_(loop, listenAddr, "EchoServer") {
    server_.setConnectionCallback(
        [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    server_.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
    server_.setThreadNum(num_threads);
  }

  void start() { server_.start(); }
  // void stop();

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
              << conn->localAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    LOG_INFO << conn->getTcpInfoString();

    conn->send("hello\n");
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    std::string msg(buf->retrieveAllAsString());
    LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at "
              << time.toString();
    if (msg == "exit\n") {
      conn->send("bye\n");
      conn->shutdown();
    }
    if (msg == "quit\n") {
      loop_->quit();
    }
    conn->send(msg);
  }

  lynx::EventLoop *loop_;
  lynx::TcpServer server_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid();
  LOG_INFO << "sizeof TcpConnection = " << sizeof(lynx::TcpConnection);
  if (argc > 1) {
    num_threads = atoi(argv[1]);
  }
  bool ipv6 = argc > 2;
  lynx::EventLoop loop;
  lynx::InetAddress listen_addr(2000, false, ipv6);
  EchoServer server(&loop, listen_addr);

  server.start();

  loop.loop();
}
