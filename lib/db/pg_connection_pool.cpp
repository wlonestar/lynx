#include "lynx/db/pg_connection_pool.h"

namespace lynx {

PgConnectionPool::PgConnectionPool(
    const std::string &host, const std::string &port, const std::string &user,
    const std::string &password, const std::string &dbname, size_t timeout,
    size_t minSize, size_t maxSize, const std::string &name)
    : host_(host), port_(port), user_(user), password_(password),
      dbname_(dbname), timeout_(timeout), min_size_(minSize),
      max_size_(maxSize), name_(name) {}

PgConnectionPool::~PgConnectionPool() {
  if (running_) {
    stop();
  }
}

void PgConnectionPool::start() {
  std::unique_lock<std::mutex> lock(mutex_);
  running_ = true;
  while (queue_.size() < min_size_) {
    auto conn = createConnection();
    queue_.push_back(conn);
  }
}

void PgConnectionPool::stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  running_ = false;
  while (!queue_.empty()) {
    std::shared_ptr<PgConnection> conn = queue_.front();
    queue_.pop_front();
    conn.reset();
  }
}

std::shared_ptr<PgConnection> PgConnectionPool::acquire() {
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

void PgConnectionPool::release(std::shared_ptr<PgConnection> conn) {
  std::unique_lock<std::mutex> lock(mutex_);
  if (queue_.size() < max_size_) {
    queue_.push_back(conn);
    not_empty_.notify_one();
  } else {
    conn.reset();
  }
}

std::shared_ptr<PgConnection> PgConnectionPool::createConnection() {
  auto conn = std::make_shared<PgConnection>();
  conn->connect(host_, port_, user_, password_, dbname_, timeout_);
  return conn;
}

} // namespace lynx
