#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_client.h"

class TimeClient : lynx::Noncopyable {
public:
  TimeClient(lynx::EventLoop *loop, const lynx::InetAddress &serverAddr)
      : loop_(loop), client_(loop, serverAddr, "TimeClient") {
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
                 lynx::Timestamp recvTime) {
    if (buf->readableBytes() >= sizeof(int32_t)) {
      const void *data = buf->peek();
      int32_t be32 = *static_cast<const int32_t *>(data);
      buf->retrieve(sizeof(int32_t));
      time_t time = be32toh(be32);
      lynx::Timestamp ts(static_cast<uint64_t>(time) *
                         lynx::Timestamp::K_MICRO_SECS_PER_SEC);
      LOG_INFO << "Server time = " << time << ", " << ts.toFormattedString();
    } else {
      LOG_INFO << conn->name() << " no enough data " << buf->readableBytes()
               << " at " << recvTime.toFormattedString();
    }
  }

  lynx::EventLoop *loop_;
  lynx::TcpClient client_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid();
  if (argc > 1) {
    lynx::EventLoop loop;
    lynx::InetAddress server_addr(argv[1], 2037);

    TimeClient time_client(&loop, server_addr);
    time_client.connect();
    loop.loop();
  } else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
}
