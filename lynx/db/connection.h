#ifndef LYNX_DB_CONNECTION_H
#define LYNX_DB_CONNECTION_H

#include "lynx/logger/logging.h"
#include "lynx/orm/key_util.h"
#include "lynx/orm/pg_query_wrapper.h"
#include "lynx/orm/traits_util.h"

#include <atomic>
#include <chrono>

namespace lynx {

namespace detail {

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
      field_types[idx] = "varchar(" + std::to_string(ArraySize<U>::value) + ")";
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

  std::string sequence_name;

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
          sequence_name =
              std::string(table_name.data()) + "_" + field_name + "_seq";
          sql.append(field_name)
              .append(" ")
              .append(field_type)
              .append(" not null default nextval('" + sequence_name + "')");
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

  /// For auto key, create sequence first
  if (!sequence_name.empty()) {
    sql = "create sequence " + sequence_name + "; " + sql;
  }

  return sql;
}

} // namespace detail

/**
 * @class Connection
 * @brief A class representing a connection to a PostgreSQL database.
 */
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

  /**
   * @brief Constructs a Connection object with the given name.
   *
   * @param name The name of the connection.
   */
  explicit Connection(const std::string &name = std::string());
  ~Connection();

  /**
   * @brief Establishes a connection to the PostgreSQL database.
   *
   * @param host The host of the database.
   * @param port The port of the database.
   * @param user The username for the database.
   * @param password The password for the database.
   * @param dbname The name of the database.
   *
   * @return True if the connection is successful, false otherwise.
   */
  bool connect(const std::string &host, size_t port, const std::string &user,
               const std::string &password, const std::string &dbname);

  /**
   * @brief Executes a SQL query on the database.
   *
   * @param sql The SQL query to execute.
   *
   * @return True if the query is successful, false otherwise.
   */
  bool execute(const std::string &sql);

  /**
   * @brief Creates a table in the database based on the given entity type.
   *
   * @tparam T The type of entity to create the table for.
   * @param args The arguments for generating the table creation SQL.
   *
   * @return True if the table creation is successful, false otherwise.
   */
  template <typename T, typename... Args> bool createTable(Args &&...args) {
    // Generate the table creation SQL
    std::string sql =
        detail::generateCreateTableSql<T>(std::forward<Args>(args)...);
    LOG_TRACE << name_ << " create: " << sql;
    // Execute the SQL query
    res_ = PQexec(conn_, sql.data());
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  /**
   * @brief Inserts a single entity into the database.
   *
   * @tparam T The type of entity to insert.
   * @param t The entity to insert.
   *
   * @return The number of rows inserted.
   */
  template <typename T> int insert(T &t) {
    return InsertWrapper<T>(conn_, getName<T>()).insert(t);
  }

  /**
   * @brief Inserts multiple entities into the database.
   *
   * @tparam T The type of entity to insert.
   * @param t The vector of entities to insert.
   *
   * @return The number of rows inserted.
   */
  template <typename T> int insert(std::vector<T> &t) {
    return InsertWrapper<T>(conn_, getName<T>()).insert(t);
  }

  /**
   * @brief Queries entities from the database.
   *
   * @tparam T The type of entity to query.
   * @tparam ID The type of the ID of entity T.
   *
   * @return A QueryResult object representing the query.
   */
  template <typename T, typename ID> constexpr QueryResult<T, ID> query() {
    return QueryWrapper<T, ID>(conn_, getName<T>());
  }

  /**
   * @brief Updates entities in the database.
   *
   * @tparam T The type of entity to update.
   * @tparam ID The type of the ID of entity T.
   *
   * @return An UpdateResult object representing the update.
   */
  template <typename T, typename ID> constexpr UpdateResult<T, ID> update() {
    return UpdateWrapper<T, ID>(conn_, getName<T>());
  }

  /**
   * @brief Deletes entities from the database.
   *
   * @tparam T The type of entity to delete.
   * @tparam ID The type of the ID of entity T.
   *
   * @return A DeleteResult object representing the deletion.
   */
  template <typename T, typename ID> constexpr DeleteResult<T, ID> del() {
    return DeleteWrapper<T, ID>(conn_, getName<T>());
  }

  /// Refreshes the alive time of the connection.
  void refreshAliveTime();

  /// Gets the alive time of the connection.
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
