#ifndef LYNX_DB_PG_CONNECTION_POOL_H
#define LYNX_DB_PG_CONNECTION_POOL_H

#include "lynx/base/noncopyable.h"
#include "lynx/db/pg_connection.h"
#include "lynx/logger/logging.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace lynx {

class PgConnectionPool : Noncopyable {
public:
  explicit PgConnectionPool(
      const std::string &host, const std::string &port, const std::string &user,
      const std::string &password, const std::string &dbname,
      const std::string &timeout = std::string("10"), size_t minSize = 5,
      size_t maxSize = 10,
      const std::string &name = std::string("PgConnectionPool"));

  ~PgConnectionPool();

  void start();
  void stop();

  std::shared_ptr<PgConnection> acquire();
  void release(std::shared_ptr<PgConnection> conn);

private:
  std::shared_ptr<PgConnection> createConnection();

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
