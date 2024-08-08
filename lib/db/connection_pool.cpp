#include "lynx/db/connection_pool.h"
#include "lynx/db/connection.h"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace lynx {

ConnectionPool::ConnectionPool(ConnectionPoolConfig &config,
                               const std::string &name)
    : config_(config), name_(name),
      produce_thread_([&] { produceConnection(); }),
      recycle_thread_([&] { recycleConnection(); }) {}

ConnectionPool::~ConnectionPool() {
  if (running_) {
    stop();
  }
}

void ConnectionPool::start() {
  for (size_t i = 0; i < config_.min_size_; i++) {
    addConnection();
    curr_size_++;
  }
  produce_thread_.start();
  recycle_thread_.start();
  running_ = true;
}

void ConnectionPool::stop() {
  produce_thread_.join();
  recycle_thread_.join();
  while (!queue_.empty()) {
    auto *conn = queue_.front();
    queue_.pop();
    delete conn;
    curr_size_--;
  }
  running_ = false;
}

std::shared_ptr<Connection> ConnectionPool::getConnection() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (queue_.empty()) {
    while (queue_.empty()) {
      if (std::cv_status::timeout ==
          cond_.wait_for(lock, std::chrono::milliseconds(config_.timeout_))) {
        if (queue_.empty()) {
          continue;
        }
      }
    }
  }

  std::shared_ptr<Connection> conn_ptr(queue_.front(), [&](Connection *conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    conn->refreshAliveTime();
    queue_.push(conn);
  });
  queue_.pop();
  cond_.notify_all();
  return conn_ptr;
}

void ConnectionPool::produceConnection() {
  while (true) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
      cond_.wait(lock);
    }

    if (curr_size_ < config_.max_size_) {
      addConnection();
      curr_size_++;
      cond_.notify_all();
    }
  }
}

void ConnectionPool::recycleConnection() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    std::lock_guard<std::mutex> lock(mutex_);
    while (queue_.size() > config_.min_size_) {
      auto *conn = queue_.front();
      if (conn->getAliveTime() >= config_.max_idle_time_) {
        queue_.pop();
        delete conn;
        curr_size_--;
      } else {
        break;
      }
    }
  }
}

void ConnectionPool::addConnection() {
  auto *conn = new Connection();
  conn->connect(config_.host_, config_.port_, config_.user_, config_.password_,
                config_.dbname_);
  conn->refreshAliveTime();
  queue_.push(conn);
}

} // namespace lynx
