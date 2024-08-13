#ifndef LYNX_ORM_PG_QUERY_WRAPPER_H
#define LYNX_ORM_PG_QUERY_WRAPPER_H

#include "lynx/logger/logging.h"
#include "lynx/orm/key_util.h"
#include "lynx/orm/traits_util.h"

#include <libpq-fe.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

namespace lynx {

namespace detail {

/**
 * @brief This function is used to set the parameter value for PostgreSQL query.
 *
 * @tparam T The type of the parameter value.
 * @param paramValues A vector of vectors of characters to store the parameter
 * values.
 * @param value The actual parameter value.
 */
template <typename T>
constexpr void setParamValue(std::vector<std::vector<char>> &paramValues,
                             T &&value) {
  using U = std::remove_const_t<std::remove_reference_t<T>>;

  /// Check if the type is a 64 bit integer
  if constexpr (std::is_same_v<U, int64_t> || std::is_same_v<U, uint64_t>) {
    std::vector<char> temp(65, 0);
    auto v_str = std::to_string(value); /// Convert the value to a string
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(temp);
  }
  /// Check if the type is an integral or floating point type
  else if constexpr (std::is_integral_v<U> || std::is_floating_point_v<U>) {
    std::vector<char> temp(20, 0);
    auto v_str = std::to_string(value); /// Convert the value to a string
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(std::move(temp));
  }
  /// Check if the type is an enum
  else if constexpr (std::is_enum_v<U>) {
    std::vector<char> temp(20, 0);
    auto v_str = std::to_string(static_cast<std::underlying_type_t<U>>(
        value)); /// Convert the value to a string
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(std::move(temp));
  }
  /// Check if the type is a std::string
  else if constexpr (std::is_same_v<U, std::string>) {
    std::vector<char> temp = {};
    std::copy(value.data(), value.data() + value.size() + 1,
              std::back_inserter(temp)); /// Copy the string to the vector
    paramValues.push_back(std::move(temp));
  }
  /// Check if the type is an array
  else if constexpr (std::is_array<U>::value) {
    std::vector<char> temp = {};
    std::copy(value, value + ArraySize<U>::value,
              std::back_inserter(temp)); /// Copy the array to the vector
    paramValues.push_back(std::move(temp));
  }
}

} // namespace detail

/**
 * @class Selectable
 * @brief A struct template representing a selectable expression in a SQL query.
 *
 * @tparam ReturnType The return type of the expression.
 */
template <typename ReturnType> struct Selectable {
  /**
   * @brief Constructs a Selectable object.
   *
   * @param field The field or column name.
   * @param tblName The table name.
   * @param op The operator to be used in the expression.
   */
  Selectable(std::string_view &&field, std::string_view &&tblName,
             std::string_view &&op)
      : expr_(std::string(op) + "(" + std::string(field) + ")"),
        tbl_name_(tblName) {}

  /// Returns the string representation of the expression.
  inline std::string toString() const { return expr_; }

  /// Returns the table name associated with the expression.
  inline std::string tableName() const { return tbl_name_; }

  /// The return type of the expression.
  ReturnType return_type; // NOLINT

private:
  std::string expr_;     /// The string representation of the expression.
  std::string tbl_name_; /// The table name associated with the expression.
};

/**
 * @class Expr
 * @brief A class representing an expression in a SQL query.
 */
class Expr {
public:
  /**
   * @brief Constructs an Expr object.
   *
   * @param field The field or column name.
   * @param tblName The table name.
   */
  explicit Expr(std::string_view &&field, std::string_view &&tblName)
      : expr_(field), tbl_name_(tblName) {}

  /**
   * @brief Creates a new expression by applying an operator to the current
   *        expression and a value.
   *
   * @tparam T The type of the value.
   * @param op The operator to be used in the expression.
   * @param value The value to be used in the expression.
   *
   * @return An Expr object representing the new expression.
   */
  template <typename T> Expr makeExpr(std::string &&op, T value) {
    using U = std::decay_t<T>;
    /// If the value is an array, a string, or a constant character pointer,
    /// wrap it in quotes and append it to the current expression.
    if constexpr (std::is_array<U>::value || std::is_same_v<U, std::string> ||
                  std::is_same_v<U, const char *>) {
      return Expr(expr_ + " " + op + " " + "'" + value + "'", tbl_name_);
    }
    /// If the value is another Expr object, append it to the current
    /// expression.
    else if constexpr (std::is_same_v<T, Expr>) {
      return Expr(expr_ + " " + op + " " + value.expr_, tbl_name_);
    }
    /// Otherwise, convert the value to a string and append it to the current
    /// expression.
    else {
      return Expr(expr_ + " " + op + " " + std::to_string(value), tbl_name_);
    }
  }

  template <typename T> inline Expr operator==(T val) {
    return Expr(makeExpr("=", val));
  }
  template <typename T> inline Expr operator=(T val) {
    return Expr(makeExpr("=", val));
  }
  template <typename T> inline Expr operator|(T val) {
    return Expr(makeExpr(",", val));
  }
  template <typename T> inline Expr operator!=(T val) {
    return Expr(makeExpr("!=", val));
  }
  template <typename T> inline Expr operator<(T val) {
    return Expr(makeExpr("<", val));
  }
  template <typename T> inline Expr operator>(T val) {
    return Expr(makeExpr(">", val));
  }
  template <typename T> inline Expr operator<=(T val) {
    return Expr(makeExpr("<=", val));
  }
  template <typename T> inline Expr operator>=(T val) {
    return Expr(makeExpr(">=", val));
  }
  inline Expr operator%(std::string &&val) {
    return Expr(makeExpr("like", val));
  }
  inline Expr operator^(std::string &&val) {
    return Expr(makeExpr("not like", val));
  }
  inline Expr operator&&(Expr &&val) { return Expr(makeExpr("and", val)); }
  inline Expr operator||(Expr &&val) { return Expr(makeExpr("or", val)); }

  inline std::string toString() const { return expr_; }
  inline std::string tableName() const { return tbl_name_; }
  inline std::string print() const { return tbl_name_ + ", " + expr_; }

private:
  std::string expr_;     /// The string representation of the expression.
  std::string tbl_name_; /// The table name associated with the expression.
};

template <typename T, typename ID> class QueryWrapper {
public:
  QueryWrapper(PGconn *conn, std::string_view tableName)
      : conn_(conn), table_name_(tableName) {}

  QueryWrapper(PGconn *conn, std::string_view tableName, T &queryResult,
               const std::string &selectSql, const std::string &whereSql,
               const std::string &groupBySql, const std::string &havingSql,
               const std::string &orderBySql, const std::string &limitSql,
               const std::string &offsetSql)
      : conn_(conn), table_name_(tableName), query_result_(queryResult),
        select_sql_(selectSql), where_sql_(whereSql), group_by_sql_(groupBySql),
        having_sql_(havingSql), order_by_sql_(orderBySql), limit_sql_(limitSql),
        offset_sql_(offsetSql) {}

  template <typename... Args> inline auto select(Args &&...args) {
    std::string sql = "select ";
    if constexpr (sizeof...(Args) > 0) {
      selectImpl(sql, std::forward<Args>(args)...);
    } else {
      sql += " * ";
    }
    sql += " from " + table_name_;
    (*this).select_sql_ = sql;
    return newQuery(std::tuple<decltype(args.return_type)...>{});
  }

  inline QueryWrapper &&set(const Expr &expr) {
    table_name_ = expr.tableName();
    (*this).set_sql_ = " set " + expr.toString();
    return std::move(*this);
  }
  inline QueryWrapper &&where(const Expr &expr) {
    table_name_ = expr.tableName();
    (*this).where_sql_ = " where (" + expr.toString() + ")";
    return std::move(*this);
  }
  inline QueryWrapper &&where(ID id) {
    std::stringstream ss;
    ss << " where (" << getAutoKey<T>() << " = " << id << ")";
    (*this).where_sql_ = ss.str();
    return std::move(*this);
  }
  inline QueryWrapper &&groupBy(const Expr &expr) {
    (*this).group_by_sql_ = " group by (" + expr.toString() + ")";
    return std::move(*this);
  }
  inline QueryWrapper &&having(const Expr &expr) {
    (*this).having_sql_ = " having (" + expr.toString() + ")";
    return std::move(*this);
  }
  inline QueryWrapper &&orderBy(const Expr &expr) {
    (*this).order_by_sql_ = " order by " + expr.toString() + " asc";
    return std::move(*this);
  }
  inline QueryWrapper &&orderByDesc(const Expr &expr) {
    (*this).order_by_sql_ = " order by " + expr.toString() + " desc";
    return std::move(*this);
  }
  inline QueryWrapper &&limit(std::size_t n) {
    (*this).limit_sql_ = " limit " + std::to_string(n);
    return std::move(*this);
  }
  inline QueryWrapper &&offset(std::size_t n) {
    (*this).offset_sql_ = " offset " + std::to_string(n);
    return std::move(*this);
  }

  std::string toString() {
    if (select_sql_.empty()) {
      select_sql_ = "select * from " + table_name_;
    }
    return select_sql_ + where_sql_ + group_by_sql_ + having_sql_ +
           order_by_sql_ + limit_sql_ + offset_sql_ + ";";
  }

  std::vector<T> toVector() { return execute<T>(toString()); }

private:
  template <typename Ty>
  constexpr std::enable_if_t<is_reflection<Ty>::value, std::vector<Ty>>
  execute(const std::string &sql) {
    std::vector<Ty> ret_vector;
    LOG_DEBUG << "query: " << sql;
    res_ = PQexec(conn_, sql.c_str());
    if (PQresultStatus(res_) != PGRES_TUPLES_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return ret_vector;
    }
    size_t ntuples = PQntuples(res_);
    for (size_t i = 0; i < ntuples; i++) {
      Ty tp = {};
      forEach(tp, [this, &tp, &i](auto item, auto field, auto j) {
        this->assignValue(tp.*item, i, static_cast<int>(decltype(j)::value));
      });
      ret_vector.push_back(std::move(tp));
    }
    PQclear(res_);
    return ret_vector;
  }

  template <typename Ty>
  constexpr std::enable_if_t<!is_reflection<Ty>::value, std::vector<Ty>>
  execute(const std::string &sql) {
    std::vector<Ty> ret_vector;
    LOG_DEBUG << "query: " << sql;
    res_ = PQexec(conn_, sql.c_str());
    if (PQresultStatus(res_) != PGRES_TUPLES_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return ret_vector;
    }
    size_t ntuples = PQntuples(res_);
    for (size_t i = 0; i < ntuples; i++) {
      Ty tp = {};
      int index = 0;
      forEach(tp, [this, &i, &index](auto &item, auto j) {
        if constexpr (is_reflection_v<std::decay_t<decltype(item)>>) {
          std::decay_t<decltype(item)> t = {};
          forEach(t, [this, &index, &t](auto elem, auto field, auto j) {
            assignValue(t.*elem, j, index++);
          });
          item = std::move(t);
        } else {
          assignValue(item, static_cast<int>(i), index++);
        }
      });
      ret_vector.push_back(tp);
    }
    PQclear(res_);
    return ret_vector;
  }

  template <typename... Args>
  inline QueryWrapper<std::tuple<Args...>, ID>
  newQuery(std::tuple<Args...> &&queryResult) {
    return QueryWrapper<std::tuple<Args...>, ID>(
        conn_, table_name_, queryResult, select_sql_, where_sql_, group_by_sql_,
        having_sql_, order_by_sql_, limit_sql_, offset_sql_);
  }

  template <typename... Args>
  inline void selectImpl(std::string &sql, Args &&...args) {
    constexpr auto size = sizeof...(Args);
    auto tp = std::make_tuple(std::forward<Args>(args)...);
    forEach(tp, [&](auto arg, auto i) {
      if constexpr (std::is_same_v<decltype(arg),
                                   Selectable<decltype(arg.return_type)>>) {
        bool same_table = arg.tableName() == table_name_;
        assert(same_table);
        (void)same_table;
        table_name_ = arg.tableName();
        sql += arg.toString();
        if (i != size - 1) {
          sql += ", ";
        }
      }
    });
  }

  template <typename Ty>
  constexpr void assignValue(Ty &&value, int row, int col) {
    using U = std::remove_const_t<std::remove_reference_t<Ty>>;
    if constexpr (std::is_integral<U>::value &&
                  !(std::is_same<U, int64_t>::value ||
                    std::is_same<U, uint64_t>::value)) {
      value = atoi(PQgetvalue(res_, row, col));
    } else if constexpr (std::is_enum_v<U>) {
      value = static_cast<U>(atoi(PQgetvalue(res_, row, col)));
    } else if constexpr (std::is_floating_point<U>::value) {
      value = atof(PQgetvalue(res_, row, col));
    } else if constexpr (std::is_same<U, int64_t>::value ||
                         std::is_same<U, uint64_t>::value) {
      value = atoll(PQgetvalue(res_, row, col));
    } else if constexpr (std::is_same<U, std::string>::value) {
      value = PQgetvalue(res_, row, col);
    } else if constexpr (std::is_array<U>::value &&
                         std::is_same<char, std::remove_pointer_t<
                                                std::decay_t<U>>>::value) {
      auto ptr = PQgetvalue(res_, row, col);
      memcpy(value, ptr, sizeof(U));
    } else {
      LOG_ERROR << "unsupported type:" << std::is_array<U>::value;
    }
  }

  PGconn *conn_;
  PGresult *res_;

  std::string table_name_;

  T query_result_;

  std::string select_sql_;
  std::string where_sql_;
  std::string group_by_sql_;
  std::string having_sql_;
  std::string order_by_sql_;
  std::string limit_sql_;
  std::string offset_sql_;
};

template <typename T, typename ID> class UpdateWrapper {
public:
  UpdateWrapper(PGconn *conn, std::string_view tableName,
                const std::string &updateSql = std::string("update"))
      : conn_(conn), table_name_(tableName),
        update_sql_(updateSql + " " + std::string(tableName)) {}

  UpdateWrapper(PGconn *conn, std::string_view tableName,
                const std::string &updateSql, const std::string &setSql,
                const std::string &whereSql)
      : conn_(conn), table_name_(tableName), update_sql_(updateSql),
        set_sql_(setSql), where_sql_(whereSql) {}

  inline UpdateWrapper &&set(const Expr &expr) {
    table_name_ = expr.tableName();
    (*this).set_sql_ = " set " + expr.toString();
    return std::move(*this);
  }

  inline UpdateWrapper &&set(T &&t) {
    std::string sql;
    size_t size = getValue<T>();
    forEach(t, [&](auto &item, auto field, auto j) {
      sql += std::string(field) + " = $" + std::to_string(++idx_);
      if (j != size - 1) {
        sql += ", ";
      }
      detail::setParamValue(param_values_, t.*item);
    });
    (*this).set_sql_ = " set " + sql;
    return std::move(*this);
  }

  inline UpdateWrapper &&where(const Expr &expr) {
    table_name_ = expr.tableName();
    (*this).where_sql_ = " where (" + expr.toString() + ")";
    return std::move(*this);
  }

  inline UpdateWrapper &&where(ID id) {
    detail::setParamValue(param_values_, id);
    (*this).where_sql_ = " where (" + std::string(getAutoKey<T>()) + " = $" +
                         std::to_string(++idx_) + ")";
    return std::move(*this);
  }

  bool execute() {
    std::string sql = toString();
    LOG_TRACE << "update prepare: " << sql;
    if (!this->prepare(sql)) {
      return false;
    }
    return updateImpl(sql);
  }

  std::string toString() { return update_sql_ + set_sql_ + where_sql_ + ";"; }

private:
  bool prepare(const std::string &sql) {
    res_ = PQprepare(conn_, "", sql.data(), static_cast<int>(getValue<T>()),
                     nullptr);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  bool updateImpl(std::string &sql) {
    if (param_values_.empty()) {
      return false;
    }
    std::vector<const char *> param_values_buf;
    param_values_buf.reserve(param_values_.size());
    for (auto &item : param_values_) {
      param_values_buf.push_back(item.data());
    }

    // For debug
    std::stringstream ss;
    ss << "params: ";
    for (size_t i = 0; i < param_values_buf.size(); i++) {
      ss << (i + 1) << " = " << *(param_values_buf.data() + i) << ", ";
    }
    LOG_DEBUG << ss.str();

    res_ = PQexecPrepared(conn_, "", static_cast<int>(param_values_.size()),
                          param_values_buf.data(), nullptr, nullptr, 0);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  PGconn *conn_;
  PGresult *res_;

  std::string table_name_;
  std::string update_sql_;
  std::string where_sql_;
  std::string set_sql_;

  std::vector<std::vector<char>> param_values_;
  size_t idx_ = 0;
};

template <typename T, typename ID> class DeleteWrapper {
public:
  DeleteWrapper(PGconn *conn, std::string_view tableName,
                const std::string &deleteSql = std::string("delete"))
      : conn_(conn), table_name_(tableName),
        delete_sql_(deleteSql + " from " + std::string(tableName)) {}

  DeleteWrapper(PGconn *conn, std::string_view tableName,
                const std::string &deleteSql, const std::string &whereSql)
      : conn_(conn), table_name_(tableName), delete_sql_(deleteSql),
        where_sql_(whereSql) {}

  inline DeleteWrapper &&where(const Expr &expr) {
    table_name_ = expr.tableName();
    (*this).where_sql_ = " where (" + expr.toString() + ")";
    return std::move(*this);
  }

  inline DeleteWrapper &&where(ID id) {
    detail::setParamValue(param_values_, id);
    (*this).where_sql_ = " where (" + std::string(getAutoKey<T>()) + " = $" +
                         std::to_string(++idx_) + ")";
    return std::move(*this);
  }

  bool execute() {
    std::string sql = toString();
    LOG_TRACE << "delete prepare: " << sql;
    if (!this->prepare(sql)) {
      return false;
    }
    return deleteImpl(sql);
  }

  std::string toString() { return delete_sql_ + where_sql_ + ";"; }

private:
  bool prepare(const std::string &sql) {
    res_ = PQprepare(conn_, "", sql.data(), static_cast<int>(getValue<T>()),
                     nullptr);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  bool deleteImpl(std::string &sql) {
    if (param_values_.empty()) {
      return false;
    }
    std::vector<const char *> param_values_buf;
    param_values_buf.reserve(param_values_.size());
    for (auto &item : param_values_) {
      param_values_buf.push_back(item.data());
    }

    // For debug
    std::stringstream ss;
    ss << "params: ";
    for (size_t i = 0; i < param_values_buf.size(); i++) {
      ss << (i + 1) << " = " << *(param_values_buf.data() + i) << ", ";
    }
    LOG_DEBUG << ss.str();

    res_ = PQexecPrepared(conn_, "", static_cast<int>(param_values_.size()),
                          param_values_buf.data(), nullptr, nullptr, 0);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  PGconn *conn_;
  PGresult *res_;

  std::string table_name_;
  std::string delete_sql_;
  std::string where_sql_;

  std::vector<std::vector<char>> param_values_;
  size_t idx_ = 0;
};

template <typename T> class InsertWrapper {
public:
  InsertWrapper(PGconn *conn, std::string_view tableName)
      : conn_(conn), table_name_(tableName) {}

  int insert(T &t) {
    std::string sql = generateInsertSql();
    LOG_TRACE << " insert prepare: " << sql;
    if (!prepare(sql)) {
      return 0;
    }
    return insertImpl(sql, t);
  }

  int insert(std::vector<T> &t) {
    std::string sql = generateInsertSql();
    LOG_TRACE << " insert prepare: " << sql;
    if (!prepare(sql)) {
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

private:
  bool execute(const std::string &sql) {
    LOG_DEBUG << "exec: " << sql;
    res_ = PQexec(conn_, sql.data());
    bool ret = PQresultStatus(res_) == PGRES_COMMAND_OK;
    PQclear(res_);
    return ret;
  }

  bool insertImpl(std::string &sql, T &t) {
    std::vector<std::vector<char>> param_values;
    forEach(t, [&](auto &item, auto field, auto j) {
      if (!isAutoKey<T>(getName<T>(j).data())) {
        detail::setParamValue(param_values, t.*item);
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

    // For debugging
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

  constexpr auto generateInsertSql() {
    std::string table_name = getName<T>().data();
    std::string sql = "insert into " + table_name + "(";

    auto field_names = getArray<T>();
    constexpr auto field_size = getValue<T>();
    for (size_t i = 0; i < field_size; i++) {
      std::string field_name = field_names[i].data();
      /// Skip if is auto key
      if (isAutoKey<T>(field_name)) {
        continue;
      }
      sql += field_name;
      if (i != field_size - 1) {
        sql += ", ";
      }
    }
    sql += ") values(";

    int idx = 0;
    for (size_t i = 0; i < field_size; i++) {
      std::string field_name = getName<T>(i).data();
      /// Skip if is auto key
      if (isAutoKey<T>(field_name)) {
        continue;
      }
      sql += "$" + std::to_string(++idx);
      if (i != field_size - 1) {
        sql += ", ";
      }
    }
    sql += ");";
    return sql;
  }

  bool prepare(const std::string &sql) {
    res_ = PQprepare(conn_, "", sql.data(), static_cast<int>(getValue<T>()),
                     nullptr);
    if (PQresultStatus(res_) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(conn_);
      return false;
    }
    PQclear(res_);
    return true;
  }

  PGconn *conn_;
  PGresult *res_;

  std::string table_name_;
};

template <typename T> struct FieldAttribute;

template <typename T, typename U> struct FieldAttribute<U T::*> {
  using type = T;
  using return_type = U;
};

template <typename U>
constexpr std::string_view getFieldName(std::string_view fullName) {
  using T = typename FieldAttribute<U>::type;
  return fullName.substr(getName<T>().length() + 2, fullName.length());
}

template <typename U>
constexpr std::string_view getTableName(std::string_view fullName) {
  using T = typename FieldAttribute<U>::type;
  return getName<T>();
}

#define VALUE(field)                                                           \
  lynx::Expr(lynx::getFieldName<decltype(&(field))>(std::string_view(#field)), \
             lynx::getTableName<decltype(&(field))>(std::string_view(#field)))

#define ORM_AGG(field, op, type)                                               \
  lynx::Selectable<type>(                                                      \
      lynx::getFieldName<decltype(&(field))>(std::string_view(#field)),        \
      lynx::getTableName<decltype(&(field))>(std::string_view(#field)), op)

#define FIELD(field)                                                           \
  ORM_AGG(field, "", lynx::FieldAttribute<decltype(&(field))>::return_type)

#define ORM_COUNT(field) ORM_AGG(field, "count", std::size_t)
#define ORM_SUM(field)                                                         \
  ORM_AGG(field, "sum", lynx::FieldAttribute<decltype(&(field))>::return_type)
#define ORM_AVG(field)                                                         \
  ORM_AGG(field, "avg", lynx::FieldAttribute<decltype(&(field))>::return_type)
#define ORM_MAX(field)                                                         \
  ORM_AGG(field, "max", lynx::FieldAttribute<decltype(&(field))>::return_type)
#define ORM_MIN(field)                                                         \
  ORM_AGG(field, "min", lynx::FieldAttribute<decltype(&(field))>::return_type)

} // namespace lynx

#endif
