
#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"
#include "lynx/net/event_loop.h"

#include "libpq-fe.h"

#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace lynx {

/// TODO: unused
class AsyncConnection {
public:
  AsyncConnection(EventLoop *loop) : loop_(loop) {}
  ~AsyncConnection() {
    channel_->disableAll();
    channel_->remove();
  }

  bool connect(const std::string &sql) {
    conn_ = PQconnectdb(sql.data());
    if (PQstatus(conn_) != CONNECTION_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }

    PQsetnonblocking(conn_, 1);

    sockfd_ = PQsocket(conn_);
    channel_ = std::make_unique<Channel>(loop_, sockfd_);
    channel_->setReadCallback([&](auto /*PH1*/) { handleRead(); });
    channel_->enableReading();
    LOG_INFO << "PQsocket: " << sockfd_;
    return true;
  }

  bool query(const std::string &sql) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (PQisBusy(conn_) == 1) {
      cond_.wait(lock);
    }
    if (PQsendQuery(conn_, sql.data()) == 0) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQflush(conn_);
    return true;
  }

  PGresult *getResult() {
    std::unique_lock<std::mutex> lock(mutex_);
    // PQflush(conn_);
    cond_.notify_one();
    return PQgetResult(conn_);
  }

private:
  void handleRead() {
    if (PQconsumeInput(conn_) == 0) {
      LOG_ERROR << PQerrorMessage(conn_);
      return;
    }
    PQflush(conn_);
  }

  EventLoop *loop_;
  int sockfd_;
  std::unique_ptr<Channel> channel_;
  mutable std::mutex mutex_;
  std::condition_variable cond_;

  PGconn *conn_ = nullptr;
};

} // namespace lynx
