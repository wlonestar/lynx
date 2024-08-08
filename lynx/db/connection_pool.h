#ifndef LYNX_DB_CONNECTION_POOL_H
#define LYNX_DB_CONNECTION_POOL_H

#include "lynx/base/thread.h"
#include "lynx/db/connection.h"
#include "lynx/net/event_loop.h"

#include <condition_variable>
#include <memory>
#include <queue>
#include <string>

namespace lynx {

struct ConnectionPoolConfig {
  ConnectionPoolConfig(const std::string &host, uint16_t port,
                       const std::string &user, const std::string &password,
                       const std::string &dbname, size_t minSize,
                       size_t maxSize, size_t timeout, size_t maxIdleTime)
      : host_(host), port_(port), user_(user), password_(password),
        dbname_(dbname), min_size_(minSize), max_size_(maxSize),
        timeout_(timeout), max_idle_time_(maxIdleTime) {}

  std::string host_;
  uint16_t port_;
  std::string user_;
  std::string password_;
  std::string dbname_;
  size_t min_size_;
  size_t max_size_;
  size_t timeout_;
  size_t max_idle_time_;
};

class ConnectionPool {
public:
  ConnectionPool(ConnectionPoolConfig &config,
                 const std::string &name = "ConnectionPool");
  ~ConnectionPool();

  void start();
  void stop();

  std::shared_ptr<Connection> getConnection();

private:
  void produceConnection();
  void recycleConnection();

  void addConnection();

  ConnectionPoolConfig config_;
  std::string name_;

  size_t curr_size_{};
  std::queue<Connection *> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;

  Thread produce_thread_;
  Thread recycle_thread_;

  bool running_ = false;
};

} // namespace lynx

#endif
