#ifndef LYNX_DB_CONNECTION_H
#define LYNX_DB_CONNECTION_H

#include "lynx/logger/Logging.h"
#include "lynx/orm/KeyUtil.h"
#include "lynx/orm/PGQueryWrapper.h"
#include "lynx/orm/TraitsUtil.h"

#include <atomic>
#include <chrono>

namespace lynx {

namespace detail {

/**
 * @brief Sorts a tuple by swapping the elements if the first element is not of
 * type KeyMap or AutoKeyMap.
 *
 * @tparam ArgTys The types of the elements in the tuple.
 * @param t The tuple to be sorted.
 *
 * @return A tuple with the elements swapped if the first element is not of type
 * KeyMap or AutoKeyMap.
 */
template <typename... ArgTys>
constexpr auto sortTuple(const std::tuple<ArgTys...> &T) {
  if constexpr (sizeof...(ArgTys) == 2) {
    auto [a, b] = T;
    /// If the first element is not of type KeyMap or AutoKeyMap
    if constexpr (!std::is_same_v<decltype(a), KeyMap> &&
                  !std::is_same_v<decltype(a), AutoKeyMap>)
      return std::make_tuple(b, a);
  }
  return T;
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
  constexpr auto FieldSize = getValue<T>();
  std::array<std::string, FieldSize> FieldTypes;
  forEach(T{}, [&](auto &Item, auto Field, auto J) {
    constexpr auto Idx = decltype(J)::value;
    using U = std::remove_reference_t<decltype(get<Idx>(std::declval<T>()))>;
    FieldTypes[Idx] = getTypeNamesImpl<U>();
  });
  return FieldTypes;
}

/**
 * @brief Generates the SQL statement for creating a table in a PostgreSQL
 * database based on the given struct type T.
 *
 * @tparam T The struct type for which to generate the SQL statement.
 * @tparam ArgTys Zero or more additional arguments to customize the table
 * generation process.
 *
 * @param args Zero or more additional arguments to customize the table
 * generation process.
 *
 * @return The SQL statement for creating the table.
 */
template <typename T, typename... ArgTys>
std::string generateCreateTableSql(ArgTys &&...Args) {
  /// Get table name and initialize SQL statement
  auto TableName = getName<T>();
  std::string Sql =
      std::string("create table if not exists ") + TableName.data() + "(";

  /// Get field names and data types
  auto FieldNames = getArray<T>();
  auto FieldTypes = getTypeNames<T>();

  /// Sort and process additional arguments
  using TT = std::tuple<std::decay_t<ArgTys>...>;
  if constexpr (sizeof...(ArgTys) > 0) {
    static_assert(
        !(HasType<KeyMap, TT>::value && HasType<AutoKeyMap, TT>::value),
        "KeyMap and AutoKeyMap cannot be used together");
  }
  std::string SequenceName;

  auto Tp = sortTuple(std::make_tuple(std::forward<ArgTys>(Args)...));
  constexpr auto FieldSize = getValue<T>();
  static_assert(FieldSize == FieldNames.size(),
                "field_size != field_names.size");
  /// Process each field
  for (size_t I = 0; I < FieldSize; I++) {
    std::string FieldName = FieldNames[I].data();
    std::string FieldType = FieldTypes[I].data();
    bool HasAdd = false;

    /// Process each additional argument for the field
    forEach(Tp, [&](auto Item, auto J) {
      if constexpr (std::is_same_v<decltype(Item), NotNullMap>) {
        if (Item.fields.find(FieldName) == Item.fields.end())
          return;
      } else {
        if (Item.fields != FieldName)
          return;
      }

      /// For auto key, set nextval sequence
      if constexpr (std::is_same_v<decltype(Item), AutoKeyMap>) {
        if (!HasAdd) {
          /// Create sequence for auto key
          SequenceName =
              std::string(TableName.data()) + "_" + FieldName + "_seq";
          Sql.append(FieldName)
              .append(" ")
              .append(FieldType)
              .append(" not null default nextval('")
              .append(SequenceName)
              .append("')");
          HasAdd = true;
        }
      }
      /// For simple key
      else if constexpr (std::is_same_v<decltype(Item), KeyMap>) {
        if (!HasAdd) {
          Sql.append(FieldName).append(" ").append(FieldType).append(
              " primary key");
          HasAdd = true;
        }
      }
      /// For not null key
      else if constexpr (std::is_same_v<decltype(Item), NotNullMap>) {
        if (!HasAdd) {
          if (Item.fields.find(FieldName) == Item.fields.end()) {
            return;
          }
          Sql.append(FieldName).append(" ").append(FieldType).append(
              " not null");
          HasAdd = true;
        }
      }
    });

    /// Add field if not added yet
    if (!HasAdd) {
      Sql.append(FieldName).append(" ").append(FieldType);
      HasAdd = true;
    }

    /// Add comma separator if not the last field
    if (I < FieldSize - 1)
      Sql += ", ";
  }
  /// Finalize SQL statement
  Sql += ");";

  /// Add sequence for auto key if needed
  if (!SequenceName.empty())
    Sql = "create sequence " + SequenceName + "; " + Sql;

  return Sql;
}

} // namespace detail

/**
 * @class Connection
 * @brief A class representing a connection to a PostgreSQL database.
 */
class Connection {
public:
  template <typename Ty, typename ID>
  using QueryResult = typename std::enable_if<is_reflection<Ty>::value,
                                              QueryWrapper<Ty, ID>>::type;
  template <typename Ty, typename ID>
  using UpdateResult = typename std::enable_if<is_reflection<Ty>::value,
                                               UpdateWrapper<Ty, ID>>::type;
  template <typename Ty, typename ID>
  using DeleteResult = typename std::enable_if<is_reflection<Ty>::value,
                                               DeleteWrapper<Ty, ID>>::type;
  template <typename Ty>
  using InsertResult = typename std::enable_if<is_reflection<Ty>::value,
                                               InsertWrapper<Ty>>::type;

  /**
   * @brief Constructs a Connection object with the given name.
   *
   * @param name The name of the connection.
   */
  explicit Connection(const std::string &Name = std::string());
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
  bool connect(const std::string &Host, size_t Port, const std::string &User,
               const std::string &Password, const std::string &Dbname);

  /**
   * @brief Executes a SQL query on the database.
   *
   * @param sql The SQL query to execute.
   *
   * @return True if the query is successful, false otherwise.
   */
  bool execute(const std::string &Sql);

  /**
   * @brief Creates a table in the database based on the given entity type.
   *
   * @tparam T The type of entity to create the table for.
   * @param args The arguments for generating the table creation SQL.
   *
   * @return True if the table creation is successful, false otherwise.
   */
  template <typename T, typename... ArgTys> bool createTable(ArgTys &&...Args) {
    std::string Sql =
        detail::generateCreateTableSql<T>(std::forward<ArgTys>(Args)...);
    LOG_TRACE << Name << " create: " << Sql;
    Res = PQexec(Conn, Sql.data());
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(Conn);
      return false;
    }
    PQclear(Res);
    return true;
  }

  /**
   * @brief Inserts a single entity into the database.
   *
   * @tparam Ty The type of entity to insert.
   * @param t The entity to insert.
   *
   * @return The number of rows inserted.
   */
  template <typename Ty> int insert(Ty &T) {
    return InsertWrapper<Ty>(Conn, getName<Ty>()).insert(T);
  }

  /**
   * @brief Inserts multiple entities into the database.
   *
   * @tparam Ty The type of entity to insert.
   * @param t The vector of entities to insert.
   *
   * @return The number of rows inserted.
   */
  template <typename Ty> int insert(std::vector<Ty> &T) {
    return InsertWrapper<Ty>(Conn, getName<Ty>()).insert(T);
  }

  /**
   * @brief Queries entities from the database.
   *
   * @tparam Ty The type of entity to query.
   * @tparam ID The type of the ID of entity Ty.
   *
   * @return A QueryResult object representing the query.
   */
  template <typename Ty, typename ID> constexpr QueryResult<Ty, ID> query() {
    return QueryWrapper<Ty, ID>(Conn, getName<Ty>());
  }

  /**
   * @brief Updates entities in the database.
   *
   * @tparam Ty The type of entity to update.
   * @tparam ID The type of the ID of entity Ty.
   *
   * @return An UpdateResult object representing the update.
   */
  template <typename Ty, typename ID> constexpr UpdateResult<Ty, ID> update() {
    return UpdateWrapper<Ty, ID>(Conn, getName<Ty>());
  }

  /**
   * @brief Deletes entities from the database.
   *
   * @tparam Ty The type of entity to delete.
   * @tparam ID The type of the ID of entity Ty.
   *
   * @return A DeleteResult object representing the deletion.
   */
  template <typename Ty, typename ID> constexpr DeleteResult<Ty, ID> del() {
    return DeleteWrapper<Ty, ID>(Conn, getName<Ty>());
  }

  /// Refreshes the alive time of the connection.
  void refreshAliveTime();

  /// Gets the alive time of the connection.
  uint64_t getAliveTime();

private:
  void setDefaultName();

  std::string Name;
  PGconn *Conn = nullptr;
  PGresult *Res = nullptr;

  std::chrono::steady_clock::time_point AliveTime;

  static std::atomic_int32_t NumCreated;
};

} // namespace lynx

#endif
