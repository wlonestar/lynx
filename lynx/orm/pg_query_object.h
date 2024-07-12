#ifndef LYNX_ORM_PG_QUERY_OBJECT_H
#define LYNX_ORM_PG_QUERY_OBJECT_H

#include "lynx/logger/logging.h"
#include "lynx/orm/reflection.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <libpq-fe.h>

namespace lynx {

template <typename RNT_TYPE> struct Selectable {
  Selectable(std::string_view &&field, std::string_view &&tbl_name,
             std::string_view &&op)
      : expr_(std::string(op) + "(" + std::string(field) + ")"),
        tbl_name_(tbl_name){};

  inline std::string toString() const { return expr_; }
  inline std::string tableName() const { return tbl_name_; }

  RNT_TYPE return_type; // NOLINT

private:
  std::string expr_;
  std::string tbl_name_;
};

class Expr {
public:
  Expr(std::string_view &&field, std::string_view &&tbl_name)
      : expr_(field), tbl_name_(tbl_name){};

  template <typename T> Expr makeExpr(std::string &&op, T value) {
    using U = std::decay_t<T>;
    if constexpr (std::is_array<U>::value || std::is_same_v<U, std::string> ||
                  std::is_same_v<U, const char *>) {
      return Expr(expr_ + " " + op + " " + "'" + value + "'", tbl_name_);
    } else if constexpr (std::is_same_v<T, Expr>) {
      return Expr(expr_ + " " + op + " " + value.expr_, tbl_name_);
    } else {
      return Expr(expr_ + " " + op + " " + std::to_string(value), tbl_name_);
    }
  }

  template <typename T> inline Expr operator==(T value) {
    return Expr(makeExpr("=", value));
  }
  template <typename T> inline Expr operator=(T value) {
    return Expr(makeExpr("=", value));
  }
  template <typename T> inline Expr operator|(T value) {
    return Expr(makeExpr(",", value));
  }
  template <typename T> inline Expr operator!=(T value) {
    return Expr(makeExpr("!=", value));
  }
  template <typename T> inline Expr operator<(T value) {
    return Expr(makeExpr("<", value));
  }
  template <typename T> inline Expr operator>(T value) {
    return Expr(makeExpr(">", value));
  }
  template <typename T> inline Expr operator<=(T value) {
    return Expr(makeExpr("<=", value));
  }
  template <typename T> inline Expr operator>=(T value) {
    return Expr(makeExpr(">=", value));
  }
  inline Expr operator%(std::string &&value) {
    return Expr(makeExpr("like", value));
  }
  inline Expr operator^(std::string &&value) {
    return Expr(makeExpr("not like", value));
  }
  inline Expr operator&&(Expr &&value) { return Expr(makeExpr("and", value)); }
  inline Expr operator||(Expr &&value) { return Expr(makeExpr("or", value)); }

  inline std::string toString() const { return expr_; }
  inline std::string tableName() const { return tbl_name_; }
  inline std::string print() const { return tbl_name_ + ", " + expr_; }

private:
  std::string expr_;
  std::string tbl_name_;
};

template <typename QueryResult> class QueryObject {
public:
  QueryObject(PGconn *conn, std::string_view table_name)
      : conn_(conn), table_name_(table_name) {}

  QueryObject(PGconn *conn, std::string_view table_name,
              const std::string &delete_sql, const std::string &update_sql)
      : conn_(conn), table_name_(table_name),
        delete_sql_(update_sql.empty()
                        ? delete_sql + " from " + std::string(table_name)
                        : ""),
        update_sql_(delete_sql.empty()
                        ? update_sql + " " + std::string(table_name)
                        : "") {}

  QueryObject(PGconn *conn, std::string_view table_name,
              QueryResult &query_result, const std::string &select_sql,
              const std::string &where_sql, const std::string &group_by_sql,
              const std::string &having_sql, const std::string &order_by_sql,
              const std::string &limit_sql, const std::string &offset_sql,
              const std::string &delete_sql, const std::string &update_sql,
              const std::string &set_sql)
      : conn_(conn), table_name_(table_name), query_result_(query_result),
        select_sql_(select_sql), where_sql_(where_sql),
        group_by_sql_(group_by_sql), having_sql_(having_sql),
        order_by_sql_(order_by_sql), limit_sql_(limit_sql),
        offset_sql_(offset_sql), delete_sql_(delete_sql),
        update_sql_(update_sql), set_sql_(set_sql) {}

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

  inline QueryObject &&set(const Expr &expression) {
    table_name_ = expression.tableName();
    (*this).set_sql_ = " set " + expression.toString();
    return std::move(*this);
  }
  inline QueryObject &&where(const Expr &expression) {
    table_name_ = expression.tableName();
    (*this).where_sql_ = " where (" + expression.toString() + ")";
    return std::move(*this);
  }
  inline QueryObject &&groupBy(const Expr &expression) {
    (*this).group_by_sql_ = " group by (" + expression.toString() + ")";
    return std::move(*this);
  }
  inline QueryObject &&having(const Expr &expression) {
    (*this).having_sql_ = " having (" + expression.toString() + ")";
    return std::move(*this);
  }
  inline QueryObject &&orderBy(const Expr &expression) {
    (*this).order_by_sql_ = " order by " + expression.toString() + " asc";
    return std::move(*this);
  }
  inline QueryObject &&orderByDesc(const Expr &expression) {
    (*this).order_by_sql_ = " order by " + expression.toString() + " desc";
    return std::move(*this);
  }
  inline QueryObject &&limit(std::size_t n) {
    (*this).limit_sql_ = " limit " + std::to_string(n);
    return std::move(*this);
  }
  inline QueryObject &&offset(std::size_t n) {
    (*this).offset_sql_ = " offset " + std::to_string(n);
    return std::move(*this);
  }

  bool execute() {
    auto sql = toString();
    LOG_DEBUG << "exec: " << sql;
    res_ = PQexec(conn_, sql.data());
    bool ret = PQresultStatus(res_) == PGRES_COMMAND_OK;
    PQclear(res_);
    return ret;
  }

  std::string toString() {
    if (select_sql_.empty() && delete_sql_.empty() && update_sql_.empty()) {
      select_sql_ = "select * from " + table_name_;
    }
    return update_sql_ + delete_sql_ + select_sql_ + set_sql_ + where_sql_ +
           group_by_sql_ + having_sql_ + order_by_sql_ + limit_sql_ +
           offset_sql_ + ";";
  }

  std::vector<QueryResult> toVector() { return query<QueryResult>(toString()); }

private:
  template <typename T>
  constexpr std::enable_if_t<is_reflection<T>::value, std::vector<T>>
  query(const std::string &sql) {
    std::vector<T> ret_vector;
    LOG_DEBUG << "query: " << sql;
    res_ = PQexec(conn_, sql.c_str());
    if (PQresultStatus(res_) != PGRES_TUPLES_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return ret_vector;
    }
    size_t ntuples = PQntuples(res_);
    for (size_t i = 0; i < ntuples; i++) {
      T tp = {};
      forEach(tp, [this, &tp, &i](auto item, auto field, auto j) {
        this->assignValue(tp.*item, i, static_cast<int>(decltype(j)::value));
      });
      ret_vector.push_back(std::move(tp));
    }
    PQclear(res_);
    return ret_vector;
  }

  template <typename T>
  constexpr std::enable_if_t<!is_reflection<T>::value, std::vector<T>>
  query(const std::string &sql) {
    std::vector<T> ret_vector;
    LOG_DEBUG << "query: " << sql;
    res_ = PQexec(conn_, sql.c_str());
    if (PQresultStatus(res_) != PGRES_TUPLES_OK) {
      LOG_ERROR << PQresultErrorMessage(res_);
      PQclear(res_);
      return ret_vector;
    }
    size_t ntuples = PQntuples(res_);
    for (size_t i = 0; i < ntuples; i++) {
      T tp = {};
      int index = 0;
      forEach(tp, [this, &i, &index](auto &item, auto j) {
        if constexpr (is_reflection_v<std::decay_t<decltype(item)>>) {
          std::decay_t<decltype(item)> t = {};
          forEach(t, [this, &index, &t](auto ele, auto field, auto j) {
            assignValue(t.*ele, j, index++);
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
  inline QueryObject<std::tuple<Args...>>
  newQuery(std::tuple<Args...> &&query_result) {
    return QueryObject<std::tuple<Args...>>(
        conn_, table_name_, query_result, select_sql_, where_sql_,
        group_by_sql_, having_sql_, order_by_sql_, limit_sql_, offset_sql_,
        delete_sql_, update_sql_, set_sql_);
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

  template <typename T>
  constexpr void assignValue(T &&value, int row, int col) {
    using U = std::remove_const_t<std::remove_reference_t<T>>;
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

  std::string table_name_;
  QueryResult query_result_;

  std::string select_sql_;
  std::string where_sql_;
  std::string group_by_sql_;
  std::string having_sql_;
  std::string order_by_sql_;
  std::string limit_sql_;
  std::string offset_sql_;
  std::string delete_sql_;
  std::string update_sql_;
  std::string set_sql_;

  PGresult *res_;
};

template <typename T> struct FieldAttribute;

template <typename T, typename U> struct FieldAttribute<U T::*> {
  using type = T;
  using return_type = U;
};

template <typename U>
constexpr std::string_view getFieldName(std::string_view full_name) {
  using T = typename FieldAttribute<U>::type;
  return full_name.substr(getName<T>().length() + 2, full_name.length());
}

template <typename U>
constexpr std::string_view getTableName(std::string_view full_name) {
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
