#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/event_loop_thread_pool.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/tcp_client.h"
#include "lynx/net/tcp_connection.h"

class Client;

class Session : lynx::Noncopyable {
public:
  Session(lynx::EventLoop *loop, const lynx::InetAddress &serverAddr,
          const std::string &name, Client *owner)
      : client_(loop, serverAddr, name), owner_(owner) {
    client_.setConnectionCallback(
        [&](auto &&PH1) { onConnection(std::forward<decltype(PH1)>(PH1)); });
    client_.setMessageCallback([&](auto &&PH1, auto &&PH2, auto &&PH3) {
      onMessage(std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
    });
  }

  void start() { client_.connect(); }
  void stop() { client_.disconnect(); }

  int64_t bytesRead() const { return bytes_read_; }
  int64_t messagesRead() const { return messages_read_; }

private:
  void onConnection(const lynx::TcpConnectionPtr &conn);

  void onMessage(const lynx::TcpConnectionPtr &conn, lynx::Buffer *buf,
                 lynx::Timestamp time) {
    ++messages_read_;
    bytes_read_ += buf->readableBytes();
    bytes_written_ += buf->readableBytes();
    conn->send(buf);
  }

  lynx::TcpClient client_;
  Client *owner_;
  int64_t bytes_read_{};
  int64_t bytes_written_{};
  int64_t messages_read_{};
};

class Client : lynx::Noncopyable {
public:
  Client(lynx::EventLoop *loop, const lynx::InetAddress &serverAddr,
         int blockSize, int sessionCount, int timeout, int threadCount)
      : loop_(loop), thread_pool_(loop, "pingpong-client"),
        session_count_(sessionCount), timeout_(timeout) {
    loop->runAfter(timeout, [&] { handleTimeout(); });

    if (threadCount > 1) {
      thread_pool_.setThreadNum(threadCount);
    }
    thread_pool_.start();

    for (int i = 0; i < blockSize; i++) {
      message_.push_back(static_cast<char>(i % 128));
    }

    for (int i = 0; i < sessionCount; i++) {
      char buf[32];
      snprintf(buf, sizeof(buf), "C%05d", i);
      auto *session =
          new Session(thread_pool_.getNextLoop(), serverAddr, buf, this);
      session->start();
      sessions_.emplace_back(session);
    }
  }

  const std::string &message() const { return message_; }

  void onConnect() {
    if (num_connected_.fetch_add(1) == session_count_) {
      LOG_WARN << "all connected";
    }
  }

  void onDisconnect(const lynx::TcpConnectionPtr &conn) {
    if (num_connected_.fetch_sub(1) == 0) {
      LOG_WARN << "all disconnected";

      int64_t total_bytes_read = 0;
      int64_t total_messages_read = 0;

      for (const auto &session : sessions_) {
        total_bytes_read += session->bytesRead();
        total_messages_read += session->messagesRead();
      }

      LOG_WARN << total_bytes_read << " total bytes read";
      LOG_WARN << total_messages_read << " total messages read";
      LOG_WARN << static_cast<double>(total_bytes_read) /
                      static_cast<double>(total_messages_read)
               << " average message size";
      LOG_WARN << static_cast<double>(total_bytes_read) /
                      (timeout_ * 1024 * 1024)
               << " MiB/s throughput";

      conn->getLoop()->queueInLoop([&] { quit(); });
    }
  }

private:
  void quit() {
    loop_->queueInLoop([capture = loop_] { capture->quit(); });
  }

  void handleTimeout() {
    LOG_WARN << "stop";
    for (auto &session : sessions_) {
      session->stop();
    }
  }

  lynx::EventLoop *loop_;
  lynx::EventLoopThreadPool thread_pool_;
  int session_count_;
  int timeout_;
  std::vector<std::unique_ptr<Session>> sessions_;
  std::string message_;
  std::atomic_int32_t num_connected_;
};

void Session::onConnection(const lynx::TcpConnectionPtr &conn) {
  if (conn->connected()) {
    conn->setTcpNoDelay(true);
    conn->send(owner_->message());
    owner_->onConnect();
  } else {
    owner_->onDisconnect(conn);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 7) {
    fprintf(
        stderr,
        "Usage: %s <host_ip> <port> <threads> <blocksize> <sessions> <time>\n",
        argv[0]);
    exit(-1);
  }

  LOG_INFO << "pid = " << getpid() << ", tid = " << lynx::current_thread::tid();

  const char *ip = argv[1];
  auto port = static_cast<uint16_t>(atoi(argv[2]));
  int thread_count = atoi(argv[3]);
  int block_size = atoi(argv[4]);
  int session_count = atoi(argv[5]);
  int timeout = atoi(argv[6]);

  lynx::EventLoop loop;
  lynx::InetAddress server_addr(ip, port);

  Client client(&loop, server_addr, block_size, session_count, timeout,
                thread_count);
  loop.loop();
}
