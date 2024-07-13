#ifndef LYNX_DB_PG_CONNECTION_POOL_H
#define LYNX_DB_PG_CONNECTION_POOL_H

#include "lynx/base/noncopyable.h"
#include "lynx/logger/logging.h"
#include "lynx/orm/pg_orm.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace lynx {

class PgConnectionPool : Noncopyable {
public:
  PgConnectionPool(const std::string &host, const std::string &port,
                   const std::string &user, const std::string &password,
                   const std::string &dbname,
                   const std::string &timeout = std::string("10"),
                   size_t minSize = 5, size_t maxSize = 10,
                   const std::string &name = std::string("PgConnectionPool"))
      : host_(host), port_(port), user_(user), password_(password),
        dbname_(dbname), timeout_(timeout), min_size_(minSize),
        max_size_(maxSize), name_(name) {}

  ~PgConnectionPool() {
    if (running_) {
      stop();
    }
  }

  void start() {
    std::unique_lock<std::mutex> lock(mutex_);
    running_ = true;
    while (queue_.size() < min_size_) {
      auto conn = createConnection();
      queue_.push_back(conn);
    }
  }

  void stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    running_ = false;
    while (!queue_.empty()) {
      std::shared_ptr<PgConnection> conn = queue_.front();
      queue_.pop_front();
      conn.reset();
    }
  }

  std::shared_ptr<PgConnection> acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.size() < min_size_ && running_) {
      not_empty_.wait(lock);
    }

    std::shared_ptr<PgConnection> conn;
    if (queue_.size() >= min_size_) {
      conn = queue_.front();
      queue_.pop_front();
    }
    while (queue_.size() < min_size_) {
      queue_.push_back(createConnection());
    }
    return conn;
  }

  void release(std::shared_ptr<PgConnection> conn) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() < max_size_) {
      queue_.push_back(conn);
      not_empty_.notify_one();
    } else {
      conn.reset();
    }
  }

private:
  std::shared_ptr<PgConnection> createConnection() {
    auto conn = std::make_shared<PgConnection>();
    conn->connect(host_, port_, user_, password_, dbname_, timeout_);
    return conn;
  }

  std::string host_;
  std::string port_;
  std::string user_;
  std::string password_;
  std::string dbname_;
  std::string timeout_;

  size_t min_size_;
  size_t max_size_;
  bool running_ = false;

  std::string name_;

  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::deque<std::shared_ptr<PgConnection>> queue_;
};

} // namespace lynx

#endif
