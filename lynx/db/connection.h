#ifndef LYNX_DB_CONNECTION_H
#define LYNX_DB_CONNECTION_H

#include "lynx/logger/logging.h"
#include "lynx/orm/key_util.h"
#include "lynx/orm/pg_query_object.h"
#include "lynx/orm/sql_util.h"
#include "lynx/orm/traits_util.h"

#include <atomic>
#include <chrono>

namespace lynx {

class Connection {
public:
  template <typename T, typename ID>
  using QueryResult = typename std::enable_if<is_reflection<T>::value,
                                              QueryWrapper<T, ID>>::type;
  template <typename T, typename ID>
  using UpdateResult = typename std::enable_if<is_reflection<T>::value,
                                               UpdateWrapper<T, ID>>::type;
  template <typename T, typename ID>
  using DeleteResult = typename std::enable_if<is_reflection<T>::value,
                                               DeleteWrapper<T, ID>>::type;
  template <typename T>
  using InsertResult =
      typename std::enable_if<is_reflection<T>::value, InsertWrapper<T>>::type;

  explicit Connection(const std::string &name = std::string());
  ~Connection();

  bool connect(const std::string &host, size_t port, const std::string &user,
               const std::string &password, const std::string &dbname);

  bool execute(const std::string &sql);

  template <typename T, typename... Args> bool createTable(Args &&...args) {
    std::string sql = generateCreateTableSql<T>(std::forward<Args>(args)...);
    LOG_TRACE << name_ << " create: " << sql;
    res_ = PQexec(conn_, sql.data());
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  template <typename T> int insert(T &t) {
    return InsertWrapper<T>(conn_, getName<T>()).insert(t);
  }
  template <typename T> int insert(std::vector<T> &t) {
    return InsertWrapper<T>(conn_, getName<T>()).insert(t);
  }

  template <typename T, typename ID> constexpr QueryResult<T, ID> query() {
    return QueryWrapper<T, ID>(conn_, getName<T>());
  }
  template <typename T, typename ID> constexpr UpdateResult<T, ID> update() {
    return UpdateWrapper<T, ID>(conn_, getName<T>());
  }
  template <typename T, typename ID> constexpr DeleteResult<T, ID> del() {
    return DeleteWrapper<T, ID>(conn_, getName<T>());
  }

  void refreshAliveTime();
  uint64_t getAliveTime();

private:
  void setDefaultName();

  std::string name_;
  PGconn *conn_ = nullptr;
  PGresult *res_ = nullptr;

  std::chrono::steady_clock::time_point alive_time_;

  static std::atomic_int32_t num_created;
};

} // namespace lynx

#endif
