#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_client.h"

class ChargenClient : lynx::Noncopyable {
public:
  ChargenClient(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr)
      : loop_(loop), client_(loop, listenAddr, "ChargenClient") {
    client_.setConnectionCallback(
        [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    client_.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
  }

  void connect() { client_.connect(); }

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (!conn->connected()) {
      loop_->quit();
    }
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp receiveTime) {
    buf->retrieveAll();
  }

  lynx::EventLoop *loop_;
  lynx::TcpClient client_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid();
  if (argc > 1) {
    lynx::EventLoop loop;
    lynx::InetAddress server_addr(argv[1], 2019);

    ChargenClient chargen_client(&loop, server_addr);
    chargen_client.connect();
    loop.loop();
  } else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
}
