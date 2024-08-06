#include "lynx/base/current_thread.h"
#include "lynx/base/timestamp.h"
#include "lynx/logger/logging.h"
#include "lynx/net/buffer.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_connection.h"
#include "lynx/net/tcp_server.h"

int num_threads = 0;

class DiscardServer {
public:
  DiscardServer(lynx::EventLoop *loop, const lynx::InetAddress &listenAddr)
      : server_(loop, listenAddr, "DiscardServer"),
        start_time_(lynx::Timestamp::now()) {
    server_.setConnectionCallback(
        [&](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    server_.setMessageCallback([&](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
    server_.setThreadNum(num_threads);
    loop->runEvery(3.0, [&] { printThroughput(); });
  }

  void start() {
    LOG_INFO << "starting " << num_threads << " threads.";
    server_.start();
  }

private:
  void onConnection(const lynx::TcpConnectionPtr &conn) {
    LOG_TRACE << conn->peerAddress().toIpPort() << " -> "
              << conn->localAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
  }

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    size_t len = buf->readableBytes();
    transferred_.fetch_add(len);
    received_messages_.fetch_add(1);
    buf->retrieveAll();
  }

  void printThroughput() {
    lynx::Timestamp end_time = lynx::Timestamp::now();
    int64_t new_counter = transferred_;
    int64_t bytes = new_counter - old_counter_;
    int64_t msgs = received_messages_.exchange(0);
    double time = lynx::timeDiff(end_time, start_time_);

    printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
           static_cast<double>(bytes) / time / 1024 / 1024,
           static_cast<double>(msgs) / time / 1024,
           static_cast<double>(bytes) / static_cast<double>(msgs));

    old_counter_ = new_counter;
    start_time_ = end_time;
  }

  lynx::TcpServer server_;
  std::atomic_int64_t transferred_;
  std::atomic_int64_t received_messages_;
  int64_t old_counter_{};
  lynx::Timestamp start_time_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid();

  if (argc > 1) {
    num_threads = atoi(argv[1]);
  }

  lynx::EventLoop loop;
  lynx::InetAddress listen_addr(2009);

  DiscardServer server(&loop, listen_addr);
  server.start();

  loop.loop();
}
