#include "lynx/base/thread.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_client.h"

#include <cstdio>
#include <unistd.h>
#include <utility>

int num_threads = 0;
class EchoClient;
std::vector<std::unique_ptr<EchoClient>> clients;
int current = 0;

class EchoClient : lynx::Noncopyable {
public:
  EchoClient(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr,
             const std::string &id)
      : loop_(loop), client_(loop, listenAddr, "EchoClient" + id) {
    client_.setConnectionCallback(
        [this](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    client_.setMessageCallback([this](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
    // client_.enableRetry();
  }

  void connect() { client_.connect(); }
  // void stop();

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected()) {
      ++current;
      if (static_cast<size_t>(current) < clients.size()) {
        clients[current]->connect();
      }
      LOG_INFO << "*** connected " << current;
    }
    conn->send("world\n");
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    std::string msg(buf->retrieveAllAsString());
    LOG_TRACE << conn->name() << " recv " << msg.size() << " bytes at "
              << time.toString();
    if (msg == "quit\n") {
      conn->send("bye\n");
      conn->shutdown();
    } else if (msg == "shutdown\n") {
      loop_->quit();
    } else {
      conn->send(msg);
    }
  }

  lynx::EventLoop *loop_;
  lynx::TcpClient client_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid();
  if (argc > 1) {
    lynx::EventLoop loop;
    bool ipv6 = argc > 3;
    lynx::InetAddress server_addr(argv[1], 2000, ipv6);

    int n = 1;
    if (argc > 2) {
      n = atoi(argv[2]);
    }

    clients.reserve(n);
    for (int i = 0; i < n; ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "%d", i + 1);
      clients.emplace_back(new EchoClient(&loop, server_addr, buf));
    }

    clients[current]->connect();
    loop.loop();
  } else {
    printf("Usage: %s host_ip [current#]\n", argv[0]);
  }
}
