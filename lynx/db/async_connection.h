#ifndef LYNX_DB_ASYNC_CONNECTION_H
#define LYNX_DB_ASYNC_CONNECTION_H

#include "lynx/logger/logging.h"
#include "lynx/net/channel.h"
#include "lynx/net/event_loop.h"

#include <libpq-fe.h>

#include <cassert>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace lynx {

class AsyncConnection {
public:
  explicit AsyncConnection(EventLoop *loop) : loop_(loop) {}
  ~AsyncConnection() {
    if (channel_) {
      channel_->disableAll();
      channel_->remove();
    }
    if (conn_ != nullptr) {
      PQfinish(conn_);
    }
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
    connected_ = true;
    return true;
  }

  bool query(const std::string &sql) {
    assert(connected_);
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
    assert(connected_);
    std::unique_lock<std::mutex> lock(mutex_);
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
    cond_.notify_all();
  }

  EventLoop *loop_;
  int sockfd_;
  std::unique_ptr<Channel> channel_;
  mutable std::mutex mutex_;
  std::condition_variable cond_;

  PGconn *conn_ = nullptr;

  bool connected_{};
};

} // namespace lynx

#endif
