#ifndef LYNX_ORM_PG_ORM_H
#define LYNX_ORM_PG_ORM_H

#include "lynx/logger/logging.h"
#include "lynx/orm/pg_query_object.h"
#include "lynx/orm/reflection.h"
#include "lynx/orm/traits_util.h"

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

class PQconnection {
public:
  // template <typename... Args> PQconnection(Args &&...args) {
  //   auto args_tp = std::make_tuple(std::forward<Args>(args)...);
  //   auto index = std::make_index_sequence<sizeof...(Args)>();
  //   constexpr size_t size = std::tuple_size<decltype(args_tp)>::value;
  //   std::string sql;
  //   if constexpr (size == 5) {
  //     auto fields =
  //         std::make_tuple("host", "port", "user", "password", "dbname");
  //     sql = generateConnectSql(fields, args_tp, index);
  //   }
  //   if constexpr (size == 6) {
  //     auto fields = std::make_tuple("host", "port", "user", "password",
  //                                   "dbname", "connect_timeout");
  //     sql = generateConnectSql(fields, args_tp, index);
  //   }
  //   LOG_DEBUG << "connect: " << sql;
  //   conn_ = PQconnectdb(sql.data());
  //   if (PQstatus(conn_) != CONNECTION_OK) {
  //     LOG_ERROR << PQerrorMessage(conn_);
  //   }
  // }

  PQconnection(const std::string &host, const std::string &port,
               const std::string &user, const std::string &password,
               const std::string &dbname) {
    auto fields = std::make_tuple("host", "port", "user", "password", "dbname");
    auto args_tp = std::make_tuple(host, port, user, password, dbname);
    auto index = std::make_index_sequence<5>();
    std::string sql = generateConnectSql(fields, args_tp, index);
    LOG_DEBUG << "Pg connect: " << sql;
    conn_ = PQconnectdb(sql.data());
    if (PQstatus(conn_) != CONNECTION_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
    }
  }

  PQconnection(const std::string &host, const std::string &port,
               const std::string &user, const std::string &password,
               const std::string &dbname, const std::string &connectTimeout) {
    auto fields = std::make_tuple("host", "port", "user", "password", "dbname",
                                  "connect_timeout");
    auto args_tp =
        std::make_tuple(host, port, user, password, dbname, connectTimeout);
    auto index = std::make_index_sequence<6>();
    std::string sql = generateConnectSql(fields, args_tp, index);
    LOG_DEBUG << "Pg connect: " << sql;
    conn_ = PQconnectdb(sql.data());
    if (PQstatus(conn_) != CONNECTION_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
    }
  }

  ~PQconnection() {
    if (conn_ != nullptr) {
      PQfinish(conn_);
      LOG_DEBUG << "Pg disconnect";
      conn_ = nullptr;
    }
  }

  template <typename T, typename... Args> bool createTable(Args &&...args) {
    std::string sql = generateCreateTableSql<T>(std::forward<Args>(args)...);
    LOG_TRACE << "create: " << sql;
    res_ = PQexec(conn_, sql.data());
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  bool execute(const std::string &sql) {
    res_ = PQexec(conn_, sql.data());
    bool ret = PQresultStatus(res_) == PGRES_COMMAND_OK;
    PQclear(res_);
    return ret;
  }

  template <typename T> int insert(T &&t) {
    std::string sql = generateInsertSql<T>(false);
    LOG_TRACE << "insert prepare: " << sql;
    if (!prepare<T>(sql)) {
      return false;
    }
    return insertImpl(sql, std::forward<T>(t));
  }

  template <typename T> int insert(std::vector<T> &t) {
    std::string sql = generateInsertSql<T>(false);
    LOG_TRACE << "insert prepare: " << sql;
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
  constexpr
      typename std::enable_if<is_reflection<T>::value, QueryObject<T>>::type
      query() {
    return QueryObject<T>(conn_, getName<T>());
  }

  template <typename T>
  constexpr
      typename std::enable_if<is_reflection<T>::value, QueryObject<T>>::type
      del() {
    return QueryObject<T>(conn_, getName<T>(), "delete", "");
  }

  template <typename T>
  constexpr
      typename std::enable_if<is_reflection<T>::value, QueryObject<T>>::type
      update() {
    return QueryObject<T>(conn_, getName<T>(), "", "update");
  }

private:
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

  template <typename T> constexpr auto getTypeNames() {
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
        field_types[idx] =
            "varchar(" + std::to_string(ArraySize<U>::value) + ")";
        return;
      }
    });
    return field_types;
  }

  template <typename T, typename... Args>
  std::string generateCreateTableSql(Args &&...args) {
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

  template <typename T> bool insertImpl(std::string &sql, T &&t) {
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

  template <typename T> constexpr auto generateInsertSql(bool replace) {
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

  template <typename T>
  constexpr void setParamValues(std::vector<std::vector<char>> &param_values,
                                T &&value) {
    using U = std::remove_const_t<std::remove_reference_t<T>>;
    if constexpr (std::is_same_v<U, int64_t> || std::is_same_v<U, uint64_t>) {
      std::vector<char> temp(65, 0);
      auto v_str = std::to_string(value);
      memcpy(temp.data(), v_str.data(), v_str.size());
      param_values.push_back(temp);
    } else if constexpr (std::is_integral_v<U> || std::is_floating_point_v<U>) {
      std::vector<char> temp(20, 0);
      auto v_str = std::to_string(value);
      memcpy(temp.data(), v_str.data(), v_str.size());
      param_values.push_back(std::move(temp));
    } else if constexpr (std::is_enum_v<U>) {
      std::vector<char> temp(20, 0);
      auto v_str =
          std::to_string(static_cast<std::underlying_type_t<U>>(value));
      memcpy(temp.data(), v_str.data(), v_str.size());
      param_values.push_back(std::move(temp));
    } else if constexpr (std::is_same_v<U, std::string>) {
      std::vector<char> temp = {};
      std::copy(value.data(), value.data() + value.size() + 1,
                std::back_inserter(temp));
      param_values.push_back(std::move(temp));
    } else if constexpr (std::is_array<U>::value) {
      std::vector<char> temp = {};
      std::copy(value, value + ArraySize<U>::value, std::back_inserter(temp));
      param_values.push_back(std::move(temp));
    }
  }

  template <typename Tuple1, typename Tuple2, size_t... Idx>
  std::string generateConnectSql(const Tuple1 &t1, const Tuple2 &t2,
                                 std::index_sequence<Idx...> /*unused*/) {
    std::stringstream os;
    auto serialize = [&os](const std::string &key, const auto &val) {
      os << key << "=" << val << " ";
    };

    int unused[] = {0, (serialize(std::get<Idx>(t1), std::get<Idx>(t2)), 0)...};
    (void)unused;
    return os.str();
  }

  PGresult *res_ = nullptr;
  PGconn *conn_ = nullptr;
};

} // namespace lynx

#endif
