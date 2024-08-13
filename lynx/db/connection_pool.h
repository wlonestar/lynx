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

/**
 * @struct ConnectionPoolConfig
 * @brief Configuration for a connection pool
 *
 * This struct holds the configuration details for a connection pool, such as
 * the database host, port, user, password, database name, minimum pool size,
 * maximum pool size, connection timeout, and maximum idle time.
 */
struct ConnectionPoolConfig {
  /**
   * @brief Constructs a new ConnectionPoolConfig object
   *
   * @param host The host of the database
   * @param port The port of the database
   * @param user The username for the database
   * @param password The password for the database
   * @param dbname The name of the database
   * @param minSize The minimum pool size
   * @param maxSize The maximum pool size
   * @param timeout The connection timeout
   * @param maxIdleTime The maximum idle time
   */
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

/**
 * @class ConnectionPool
 * @brief Connection pool for managing database connections
 *
 * This class manages a pool of database connections. It provides methods to
 * start and stop the pool, get connections from the pool, and recycle
 * connections back to the pool.
 */
class ConnectionPool {
public:
  /**
   * @brief Constructs a new ConnectionPool object
   *
   * @param config The configuration for the connection pool
   * @param name The name of the connection pool
   */
  ConnectionPool(ConnectionPoolConfig &config,
                 const std::string &name = "ConnectionPool");
  ~ConnectionPool();

  /// Starts the connection pool
  void start();

  /// Stops the connection pool
  void stop();

  /// Gets a connection from the connection pool
  std::shared_ptr<Connection> getConnection();

private:
  /// Produces a new connection and adds it to the pool
  void produceConnection();

  /// Recycles a connection back to the pool
  void recycleConnection();

  /// Adds a connection to the pool
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
