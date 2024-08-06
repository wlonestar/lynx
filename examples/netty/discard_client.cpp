#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"
#include "lynx/logger/logging.h"
#include "lynx/net/buffer.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_client.h"
#include "lynx/net/tcp_connection.h"
#include "lynx/net/tcp_server.h"

class DiscardClient {
public:
  DiscardClient(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr,
                int size)
      : loop_(loop), client_(loop, listenAddr, "DiscardClient"),
        message_(size, 'H') {
    client_.setConnectionCallback(
        [&](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    client_.setMessageCallback([&](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
    client_.setWriteCompleteCallback(
        [&](auto &&PH1) { onWriteComplete(std::forward<decltype(PH1)>(PH1)); });
  }

  void connect() { client_.connect(); }

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
      conn->setTcpNoDelay(true);
      conn->send(message_);
    } else {
      loop_->quit();
    }
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    buf->retrieveAll();
  }

  void onWriteComplete(const lynx::TcpConnectionPtr &conn) {
    // LOG_INFO << "write complete " << message_.size();
    conn->send(message_);
  }

  lynx::EventLoop *loop_;
  lynx::TcpClient client_;
  std::string message_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid();

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <host_ip> [msg_size]\n", argv[0]);
    exit(-1);
  }

  lynx::EventLoop loop;
  lynx::InetAddress server_addr(argv[1], 2009);

  int size = 256;
  if (argc > 2) {
    size = atoi(argv[2]);
  }

  DiscardClient client(&loop, server_addr, size);
  client.connect();

  loop.loop();
}
