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
  /// Create the minimum number of connections
  for (size_t i = 0; i < config_.min_size_; i++) {
    addConnection();
    curr_size_++;
  }

  /// Start the threads responsible for producing connections and recycling them
  produce_thread_.start();
  recycle_thread_.start();
  running_ = true;
}

void ConnectionPool::stop() {
  /// Wait for the producer and recycler threads to finish
  produce_thread_.join();
  recycle_thread_.join();

  /// Delete all the connections in the queue
  while (!queue_.empty()) {
    auto *conn = queue_.front();
    queue_.pop();
    delete conn;
    curr_size_--;
  }
  running_ = false;
}

std::shared_ptr<Connection> ConnectionPool::acquire() {
  std::unique_lock<std::mutex> lock(mutex_);
  /// If the queue is empty, wait for a connection to be released.
  if (queue_.empty()) {
    while (queue_.empty()) {
      /// Wait for a specified timeout duration.
      if (std::cv_status::timeout ==
          cond_.wait_for(lock, std::chrono::milliseconds(config_.timeout_))) {
        /// If the queue is still empty, continue waiting.
        if (queue_.empty()) {
          continue;
        }
      }
    }
  }

  /// Create a shared pointer to the front of the queue and wrap it in a deleter
  /// that returns the connection to the pool upon destruction.
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
      /// If the pool is not empty, wait until a connection is released
      cond_.wait(lock);
    }

    /// If the current size is less than the maximum size, produce a new
    /// connection
    if (curr_size_ < config_.max_size_) {
      addConnection();
      curr_size_++;
      /// Notify other threads that a new connection is available
      cond_.notify_all();
    }
  }
}

void ConnectionPool::recycleConnection() {
  while (true) {
    /// Sleep for 500 microseconds between checks
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    std::lock_guard<std::mutex> lock(mutex_);
    while (queue_.size() > config_.min_size_) {
      /// Get the front of the queue
      auto *conn = queue_.front();
      /// If the connection has been idle for longer than the max idle time,
      /// remove it from the queue and close it
      if (conn->getAliveTime() >= config_.max_idle_time_) {
        queue_.pop();
        delete conn;
        curr_size_--;
      } else {
        /// If the connection is not idle, break out of the loop
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
