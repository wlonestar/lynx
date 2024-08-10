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

  template <typename T> bool prepare(const std::string &sql) {
    res_ = PQprepare(conn_, "", sql.data(), static_cast<int>(getValue<T>()),
                     nullptr);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  template <typename T> int insert(T &&t) {
    std::string sql = generateInsertSql<T>();
    LOG_TRACE << name_ << " insert prepare: " << sql;
    if (!prepare<T>(sql)) {
      return false;
    }
    return insertImpl(sql, std::forward<T>(t));
  }
  template <typename T> int insert(std::vector<T> &t) {
    std::string sql = generateInsertSql<T>();
    LOG_TRACE << name_ << " insert prepare: " << sql;
    if (!prepare<T>(sql)) {
      return 0;
    }

    for (auto &item : t) {
      if (!insertImpl(sql, item)) {
        execute("rollback;");
        return 0;
      }
    }
    if (!execute("commit;")) {
      return 0;
    }
    return t.size();
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
  template <typename T> bool insertImpl(std::string &sql, T &&t) {
    std::vector<std::vector<char>> param_values;
    forEach(t, [&](auto &item, auto field, auto j) {
      if (!isAutoKey<T>(getName<T>(j).data())) {
        setParamValue(param_values, t.*item);
      }
    });
    if (param_values.empty()) {
      return false;
    }

    std::vector<const char *> param_values_buf;
    param_values_buf.reserve(param_values.size());
    for (auto &item : param_values) {
      param_values_buf.push_back(item.data());
    }

    // For debug
    std::stringstream ss;
    ss << "params: ";
    for (size_t i = 0; i < param_values_buf.size(); i++) {
      ss << i << "=" << *(param_values_buf.data() + i) << ", ";
    }
    LOG_DEBUG << ss.str();

    res_ = PQexecPrepared(conn_, "", static_cast<int>(param_values.size()),
                          param_values_buf.data(), nullptr, nullptr, 0);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  void setDefaultName();

  std::string name_;
  PGconn *conn_ = nullptr;
  PGresult *res_ = nullptr;
  std::chrono::steady_clock::time_point alive_time_;

  static std::atomic_int32_t num_created;
};

} // namespace lynx

#endif