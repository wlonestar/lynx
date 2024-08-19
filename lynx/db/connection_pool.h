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
  explicit ConnectionPool(ConnectionPoolConfig &config,
                          const std::string &name = "ConnectionPool");
  ~ConnectionPool();

  /**
   * @brief Starts the connection pool by creating the minimum number of
   * connections.
   *
   * This function creates the minimum number of connections specified in the
   * configuration and adds them to the connection pool.
   */
  void start();

  /**
   * @brief Stops the connection pool by joining the threads and deleting all
   * the connections in the queue.
   *
   * This function waits for the producer and recycler threads to finish and
   * then deletes all the connections in the queue.
   */
  void stop();

  /**
   * @brief Acquires a connection from the pool.
   *
   * This function tries to acquire a connection from the pool. If the pool is
   * empty, it waits for a specified timeout duration until a connection is
   * available. If the queue is still empty after the timeout, it returns a
   * connection from the front of the queue.
   *
   * @return A shared pointer to the acquired connection.
   */
  std::shared_ptr<Connection> acquire();

private:
  /**
   * @brief Produces a new connection and adds it to the pool
   *
   * This method runs in a separate thread and continuously checks
   * the connection pool. If the pool is empty or the current size
   * is less than the maximum size, it produces a new connection.
   */
  void produceConnection();

  /**
   * @brief Recycles a connection in the pool.
   *
   * This function is a loop that periodically checks the pool for connections
   * that are idle for longer than the maximum idle time. If a connection is
   * found, it is removed from the pool and closed.
   */
  void recycleConnection();

  /**
   * @brief Adds a new connection to the pool
   *
   * This function creates a new Connection object, connects it to the database,
   * and adds it to the connection pool.
   */
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
