#include "lynx/db/Connection.h"
#include "lynx/db/ConnectionPool.h"

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace lynx {

ConnectionPool::ConnectionPool(ConnectionPoolConfig &Config,
                               const std::string &Name)
    : Config(Config), Name(Name), ProduceThread([&] { threadProduce(); }),
      RecycleThread([&] { threadRecycle(); }), Latch(2) {}

ConnectionPool::~ConnectionPool() {
  if (Running)
    stop();
}

void ConnectionPool::start() {
  Running = true;
  for (size_t I = 0; I < Config.MinSize; I++) {
    addConnection();
    CurrSize++;
  }
  ProduceThread.start();
  RecycleThread.start();
  Latch.wait();
}

void ConnectionPool::stop() {
  Running = false;
  Cond.notify_all();
  /// Wait for the producer and recycler threads to finish
  ProduceThread.join();
  RecycleThread.join();

  /// Delete all the connections in the queue
  while (!Queue.empty()) {
    auto *Conn = Queue.front();
    Queue.pop();
    delete Conn;
    CurrSize--;
  }
}

std::shared_ptr<Connection> ConnectionPool::acquire() {
  std::unique_lock<std::mutex> Lock(Mutex);
  /// If the queue is empty, wait for a connection to be released.
  if (Queue.empty()) {
    while (Queue.empty()) {
      /// Wait for a specified timeout duration.
      if (std::cv_status::timeout ==
          Cond.wait_for(Lock, std::chrono::milliseconds(Config.Timeout))) {
        /// If the queue is still empty, continue waiting.
        if (Queue.empty())
          continue;
      }
    }
  }

  /// Create a shared pointer to the front of the queue and wrap it in a deleter
  /// that returns the connection to the pool upon destruction.
  std::shared_ptr<Connection> ConnPtr(Queue.front(), [&](Connection *Conn) {
    std::lock_guard<std::mutex> Lock(Mutex);
    Conn->refreshAliveTime();
    Queue.push(Conn);
  });
  Queue.pop();
  Cond.notify_all();
  return ConnPtr;
}

void ConnectionPool::threadProduce() {
  assert(Running == true);
  Latch.count_down();
  while (Running) {
    std::unique_lock<std::mutex> Lock(Mutex);
    while (!Queue.empty()) {
      /// If the pool is not empty, wait until a connection is released
      Cond.wait(Lock);
      if (!Running)
        break;
    }

    /// If the current size is less than the maximum size, produce a new
    /// connection
    if (CurrSize < Config.MaxSize) {
      addConnection();
      CurrSize++;
      /// Notify other threads that a new connection is available
      Cond.notify_all();
    }
  }
}

void ConnectionPool::threadRecycle() {
  assert(Running == true);
  Latch.count_down();
  while (Running) {
    /// Sleep for 500 microseconds between checks
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    if (!Running)
      break;

    std::lock_guard<std::mutex> Lock(Mutex);
    while (Queue.size() > Config.MinSize) {
      /// Get the front of the queue
      auto *Conn = Queue.front();
      /// If the connection has been idle for longer than the max idle time,
      /// remove it from the queue and close it
      if (Conn->getAliveTime() >= Config.MaxIdleTime) {
        Queue.pop();
        delete Conn;
        CurrSize--;
      } else {
        /// If the connection is not idle, break out of the loop
        break;
      }
    }
  }
}

void ConnectionPool::addConnection() {
  auto *Conn = new Connection();
  Conn->connect(Config.Host, Config.Port, Config.User, Config.Password,
                Config.Dbname);
  Conn->refreshAliveTime();
  Queue.push(Conn);
}

} // namespace lynx
