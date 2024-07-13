#ifndef LYNX_DB_PG_CONNECTION_H
#define LYNX_DB_PG_CONNECTION_H

#include "lynx/logger/logging.h"
#include "lynx/orm/pg_query_object.h"
#include "lynx/orm/reflection.h"
#include "lynx/orm/traits_util.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>

namespace lynx {

struct KeyMap {
  std::string fields; // NOLINT
};

struct AutoKeyMap {
  std::string fields; // NOLINT
};

struct NotNullMap {
  std::set<std::string> fields; // NOLINT
};

template <typename... Args>
constexpr auto sortTuple(const std::tuple<Args...> &t) {
  if constexpr (sizeof...(Args) == 2) {
    auto [a, b] = t;
    if constexpr (!std::is_same_v<decltype(a), KeyMap> &&
                  !std::is_same_v<decltype(a), AutoKeyMap>) {
      return std::make_tuple(b, a);
    }
  }
  return t;
}

class PgConnection {
public:
  template <typename T>
  using transaction_type =
      typename std::enable_if<is_reflection<T>::value, QueryObject<T>>::type;

  explicit PgConnection(const std::string &name = std::string());
  ~PgConnection();

  bool connect(const std::string &host, const std::string &port,
               const std::string &user, const std::string &password,
               const std::string &dbname,
               const std::string &connectTimeout = std::string("10"));

  bool execute(const std::string &sql);

  template <typename T, typename... Args> bool createTable(Args &&...args);

  template <typename T> bool prepare(const std::string &sql);

  template <typename T> int insert(T &&t);
  template <typename T> int insert(std::vector<T> &t);

  template <typename T> constexpr transaction_type<T> query();
  template <typename T> constexpr transaction_type<T> del();
  template <typename T> constexpr transaction_type<T> update();

private:
  template <typename T> bool insertImpl(std::string &sql, T &&t);

  template <typename Tuple1, typename Tuple2, size_t... Idx>
  std::string generateConnectSql(const Tuple1 &t1, const Tuple2 &t2,
                                 std::index_sequence<Idx...> /*unused*/);

  template <typename T, typename... Args>
  std::string generateCreateTableSql(Args &&...args);

  template <typename T> constexpr auto generateInsertSql(bool replace);

  template <typename T> constexpr auto getTypeNames();

  template <typename T>
  constexpr void setParamValues(std::vector<std::vector<char>> &paramValues,
                                T &&value);

  void setDefaultName();

  std::string name_;
  PGresult *res_ = nullptr;
  PGconn *conn_ = nullptr;

  static std::atomic_int32_t num_created;
};

template <typename T, typename... Args>
bool PgConnection::createTable(Args &&...args) {
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

template <typename T> bool PgConnection::prepare(const std::string &sql) {
  res_ = PQprepare(conn_, "", sql.data(), static_cast<int>(getValue<T>()),
                   nullptr);
  if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
    LOG_ERROR << PQerrorMessage(conn_);
    return false;
  }
  PQclear(res_);
  return true;
}

template <typename T> int PgConnection::insert(T &&t) {
  std::string sql = generateInsertSql<T>(false);
  LOG_TRACE << name_ << " insert prepare: " << sql;
  if (!prepare<T>(sql)) {
    return false;
  }
  return insertImpl(sql, std::forward<T>(t));
}

template <typename T> int PgConnection::insert(std::vector<T> &t) {
  std::string sql = generateInsertSql<T>(false);
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

template <typename T>
constexpr PgConnection::transaction_type<T> PgConnection::query() {
  return QueryObject<T>(conn_, getName<T>());
}

template <typename T>
constexpr PgConnection::transaction_type<T> PgConnection::del() {
  return QueryObject<T>(conn_, getName<T>(), "delete", "");
}

template <typename T>
constexpr PgConnection::transaction_type<T> PgConnection::update() {
  return QueryObject<T>(conn_, getName<T>(), "", "update");
}

template <typename T> bool PgConnection::insertImpl(std::string &sql, T &&t) {
  std::vector<std::vector<char>> param_values;
  forEach(t, [&](auto &item, auto field, auto j) {
    setParamValues(param_values, t.*item);
  });
  if (param_values.empty()) {
    return false;
  }
  std::vector<const char *> param_values_buf;
  param_values_buf.reserve(param_values.size());
  for (auto &item : param_values) {
    param_values_buf.push_back(item.data());
  }

  std::cout << "params: ";
  for (size_t i = 0; i < param_values_buf.size(); i++) {
    std::cout << i << "=" << *(param_values_buf.data() + i) << ", ";
  }
  std::cout << std::endl;
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

template <typename Tuple1, typename Tuple2, size_t... Idx>
std::string
PgConnection::generateConnectSql(const Tuple1 &t1, const Tuple2 &t2,
                                 std::index_sequence<Idx...> /*unused*/) {
  std::stringstream os;
  auto serialize = [&os](const std::string &key, const auto &val) {
    os << key << "=" << val << " ";
  };

  int unused[] = {0, (serialize(std::get<Idx>(t1), std::get<Idx>(t2)), 0)...};
  (void)unused;
  return os.str();
}

template <typename T, typename... Args>
std::string PgConnection::generateCreateTableSql(Args &&...args) {
  auto table_name = getName<T>();
  std::string sql =
      std::string("create table if not exists ") + table_name.data() + "(";
  auto field_names = getArray<T>();
  auto field_types = getTypeNames<T>();
  using TT = std::tuple<std::decay_t<Args>...>;
  if constexpr (sizeof...(Args) > 0) {

    static_assert(
        !(HasType<KeyMap, TT>::value && HasType<AutoKeyMap, TT>::value),
        "KeyMap and AutoKeyMap cannot be used together");
  }

  auto tp = sortTuple(std::make_tuple(std::forward<Args>(args)...));
  constexpr auto field_size = getValue<T>();
  static_assert(field_size == field_names.size(),
                "field_size != field_names.size");
  for (size_t i = 0; i < field_size; i++) {
    std::string field_name = field_names[i].data();
    std::string field_type = field_types[i].data();
    bool has_add = false;
    forEach(tp, [&](auto item, auto j) {
      if constexpr (std::is_same_v<decltype(item), NotNullMap>) {
        if (item.fields.find(field_name) == item.fields.end()) {
          return;
        }
      } else {
        if (item.fields != field_name) {
          return;
        }
      }

      if constexpr (std::is_same_v<decltype(item), AutoKeyMap>) {
        if (!has_add) {
          sql.append(field_name)
              .append(" ")
              .append(field_type)
              .append(" serial primary key");
          has_add = true;
        }
      } else if constexpr (std::is_same_v<decltype(item), KeyMap>) {
        if (!has_add) {
          sql.append(field_name)
              .append(" ")
              .append(field_type)
              .append(" primary key");
          has_add = true;
        }
      } else if constexpr (std::is_same_v<decltype(item), NotNullMap>) {
        if (!has_add) {
          if (item.fields.find(field_name) == item.fields.end()) {
            return;
          }
          sql.append(field_name)
              .append(" ")
              .append(field_type)
              .append(" not null");
          has_add = true;
        }
      }
    });

    if (!has_add) {
      sql.append(field_name).append(" ").append(field_type);
      has_add = true;
    }

    if (i < field_size - 1) {
      sql += ", ";
    }
  }
  sql += ");";
  return sql;
}

template <typename T>
constexpr auto PgConnection::generateInsertSql(bool replace) {
  std::string sql = replace ? "replace into " : "insert into ";
  std::string table_name = getName<T>().data();
  std::string field_name_pack = getField<T>().data();
  sql += table_name + "(" + field_name_pack + ") values(";
  constexpr auto field_size = getValue<T>();
  for (size_t i = 0; i < field_size; i++) {
    sql += "$";
    sql += std::to_string(i + 1);
    if (i != field_size - 1) {
      sql += ", ";
    }
  }
  sql += ");";
  return sql;
}

template <typename T> constexpr auto PgConnection::getTypeNames() {
  constexpr auto field_size = getValue<T>();
  std::array<std::string, field_size> field_types;
  forEach(T{}, [&](auto &item, auto field, auto j) {
    constexpr auto idx = decltype(j)::value;
    using U = std::remove_reference_t<decltype(get<idx>(std::declval<T>()))>;
    if constexpr (std::is_same_v<U, bool> || std::is_same_v<U, int> ||
                  std::is_same_v<U, int32_t> || std::is_same_v<U, uint32_t> ||
                  std::is_enum_v<U>) {
      field_types[idx] = "integer";
      return;
    }
    if constexpr (std::is_same_v<U, int8_t> || std::is_same_v<U, uint8_t> ||
                  std::is_same_v<U, int16_t> || std::is_same_v<U, uint16_t>) {
      field_types[idx] = "smallint";
      return;
    }
    if constexpr (std::is_same_v<U, int64_t> || std::is_same_v<U, uint64_t>) {
      field_types[idx] = "bigint";
      return;
    }
    if constexpr (std::is_same_v<U, float>) {
      field_types[idx] = "real";
      return;
    }
    if constexpr (std::is_same_v<U, double>) {
      field_types[idx] = "double precision";
      return;
    }
    if constexpr (std::is_same_v<U, std::string>) {
      field_types[idx] = "text";
      return;
    }
    if constexpr (std::is_array<U>::value) {
      field_types[idx] = "varchar(" + std::to_string(ArraySize<U>::value) + ")";
      return;
    }
  });
  return field_types;
}

template <typename T>
constexpr void
PgConnection::setParamValues(std::vector<std::vector<char>> &paramValues,
                             T &&value) {
  using U = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (std::is_same_v<U, int64_t> || std::is_same_v<U, uint64_t>) {
    std::vector<char> temp(65, 0);
    auto v_str = std::to_string(value);
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(temp);
  } else if constexpr (std::is_integral_v<U> || std::is_floating_point_v<U>) {
    std::vector<char> temp(20, 0);
    auto v_str = std::to_string(value);
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(std::move(temp));
  } else if constexpr (std::is_enum_v<U>) {
    std::vector<char> temp(20, 0);
    auto v_str = std::to_string(static_cast<std::underlying_type_t<U>>(value));
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(std::move(temp));
  } else if constexpr (std::is_same_v<U, std::string>) {
    std::vector<char> temp = {};
    std::copy(value.data(), value.data() + value.size() + 1,
              std::back_inserter(temp));
    paramValues.push_back(std::move(temp));
  } else if constexpr (std::is_array<U>::value) {
    std::vector<char> temp = {};
    std::copy(value, value + ArraySize<U>::value, std::back_inserter(temp));
    paramValues.push_back(std::move(temp));
  }
}

} // namespace lynx

#endif
