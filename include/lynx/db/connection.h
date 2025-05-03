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

/**
 * @brief Sorts a tuple by swapping the elements if the first element is not of
 * type KeyMap or AutoKeyMap.
 *
 * @tparam Args The types of the elements in the tuple.
 * @param t The tuple to be sorted.
 *
 * @return A tuple with the elements swapped if the first element is not of type
 * KeyMap or AutoKeyMap.
 */
template <typename... Args>
constexpr auto sortTuple(const std::tuple<Args...> &t) {
  if constexpr (sizeof...(Args) == 2) {
    auto [a, b] = t;
    /// If the first element is not of type KeyMap or AutoKeyMap
    if constexpr (!std::is_same_v<decltype(a), KeyMap> &&
                  !std::is_same_v<decltype(a), AutoKeyMap>) {
      return std::make_tuple(b, a);
    }
  }
  return t;
}

/**
 * @brief Returns the PostgreSQL data type name for the given type U.
 *
 * @tparam U The type for which to get the PostgreSQL data type name.
 *
 * @return The PostgreSQL data type name as a string literal.
 */
template <typename U> constexpr auto getTypeNamesImpl() {
  if constexpr (std::is_same_v<U, bool> || std::is_same_v<U, int> ||
                std::is_same_v<U, int32_t> || std::is_same_v<U, uint32_t> ||
                std::is_enum_v<U>) {
    return "integer";
  }
  if constexpr (std::is_same_v<U, int8_t> || std::is_same_v<U, uint8_t> ||
                std::is_same_v<U, int16_t> || std::is_same_v<U, uint16_t>) {
    return "smallint";
  }
  if constexpr (std::is_same_v<U, int64_t> || std::is_same_v<U, uint64_t>) {
    return "bigint";
  }
  if constexpr (std::is_same_v<U, float>) {
    return "real";
  }
  if constexpr (std::is_same_v<U, double>) {
    return "double precision";
  }
  if constexpr (std::is_same_v<U, std::string>) {
    return "text";
  }
  if constexpr (std::is_array<U>::value) {
    return "varchar(" + std::to_string(ArraySize<U>::value) + ")";
  }
}

/**
 * @brief Returns an array of PostgreSQL data type names for each field in the
 * given struct type T.
 *
 * @tparam T The struct type for which to get the PostgreSQL data type names.
 *
 * @return An array of PostgreSQL data type names as string literals.
 */
template <typename T> constexpr auto getTypeNames() {
  constexpr auto field_size = getValue<T>();
  std::array<std::string, field_size> field_types;
  forEach(T{}, [&](auto &item, auto field, auto j) {
    constexpr auto idx = decltype(j)::value;
    using U = std::remove_reference_t<decltype(get<idx>(std::declval<T>()))>;
    field_types[idx] = getTypeNamesImpl<U>();
  });
  return field_types;
}

/**
 * @brief Generates the SQL statement for creating a table in a PostgreSQL
 * database based on the given struct type T.
 *
 * @tparam T The struct type for which to generate the SQL statement.
 * @tparam Args Zero or more additional arguments to customize the table
 * generation process.
 *
 * @param args Zero or more additional arguments to customize the table
 * generation process.
 *
 * @return The SQL statement for creating the table.
 */
template <typename T, typename... Args>
std::string generateCreateTableSql(Args &&...args) {
  /// Get table name and initialize SQL statement
  auto table_name = getName<T>();
  std::string sql =
      std::string("create table if not exists ") + table_name.data() + "(";

  /// Get field names and data types
  auto field_names = getArray<T>();
  auto field_types = getTypeNames<T>();

  /// Sort and process additional arguments
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
  /// Process each field
  for (size_t i = 0; i < field_size; i++) {
    std::string field_name = field_names[i].data();
    std::string field_type = field_types[i].data();
    bool has_add = false;

    /// Process each additional argument for the field
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

      /// For auto key, set nextval sequence
      if constexpr (std::is_same_v<decltype(item), AutoKeyMap>) {
        if (!has_add) {
          /// Create sequence for auto key
          sequence_name =
              std::string(table_name.data()) + "_" + field_name + "_seq";
          sql.append(field_name)
              .append(" ")
              .append(field_type)
              .append(" not null default nextval('")
              .append(sequence_name)
              .append("')");
          has_add = true;
        }
      }
      /// For simple key
      else if constexpr (std::is_same_v<decltype(item), KeyMap>) {
        if (!has_add) {
          sql.append(field_name)
              .append(" ")
              .append(field_type)
              .append(" primary key");
          has_add = true;
        }
      }
      /// For not null key
      else if constexpr (std::is_same_v<decltype(item), NotNullMap>) {
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

    /// Add field if not added yet
    if (!has_add) {
      sql.append(field_name).append(" ").append(field_type);
      has_add = true;
    }

    /// Add comma separator if not the last field
    if (i < field_size - 1) {
      sql += ", ";
    }
  }
  /// Finalize SQL statement
  sql += ");";

  /// Add sequence for auto key if needed
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
    std::string sql =
        detail::generateCreateTableSql<T>(std::forward<Args>(args)...);
    LOG_TRACE << name_ << " create: " << sql;
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
