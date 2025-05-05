#ifndef LYNX_ORM_PG_QUERY_WRAPPER_H
#define LYNX_ORM_PG_QUERY_WRAPPER_H

#include "lynx/logger/Logging.h"
#include "lynx/orm/KeyUtil.h"
#include "lynx/orm/TraitsUtil.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

#include <libpq-fe.h>

namespace lynx {

namespace detail {

/**
 * @brief This function is used to set the parameter Value for PostgreSQL query.
 *
 * @tparam Ty The type of the parameter Value.
 * @param paramValues A vector of vectors of characters to store the parameter
 * values.
 * @param Value The actual parameter Value.
 */
template <typename Ty>
constexpr void setParamValue(std::vector<std::vector<char>> &ParamValues,
                             Ty &&Value) {
  using U = std::remove_const_t<std::remove_reference_t<Ty>>;

  /// Check if the type is a 64 bit integer
  if constexpr (std::is_same_v<U, int64_t> || std::is_same_v<U, uint64_t>) {
    std::vector<char> Temp(65, 0);
    auto VStr = std::to_string(Value); /// Convert the Value to a string
    memcpy(Temp.data(), VStr.data(), VStr.size());
    ParamValues.push_back(Temp);
  }
  /// Check if the type is an integral or floating point type
  else if constexpr (std::is_integral_v<U> || std::is_floating_point_v<U>) {
    std::vector<char> Temp(20, 0);
    auto VStr = std::to_string(Value); /// Convert the Value to a string
    memcpy(Temp.data(), VStr.data(), VStr.size());
    ParamValues.push_back(std::move(Temp));
  }
  /// Check if the type is an enum
  else if constexpr (std::is_enum_v<U>) {
    std::vector<char> Temp(20, 0);
    auto VStr = std::to_string(static_cast<std::underlying_type_t<U>>(
        Value)); /// Convert the Value to a string
    memcpy(Temp.data(), VStr.data(), VStr.size());
    ParamValues.push_back(std::move(Temp));
  }
  /// Check if the type is a std::string
  else if constexpr (std::is_same_v<U, std::string>) {
    std::vector<char> Temp = {};
    std::copy(Value.data(), Value.data() + Value.size() + 1,
              std::back_inserter(Temp)); /// Copy the string to the vector
    ParamValues.push_back(std::move(Temp));
  }
  /// Check if the type is an array
  else if constexpr (std::is_array<U>::value) {
    std::vector<char> Temp = {};
    std::copy(Value, Value + ArraySize<U>::value,
              std::back_inserter(Temp)); /// Copy the array to the vector
    ParamValues.push_back(std::move(Temp));
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
   * @param Field The Field or column name.
   * @param tblName The table name.
   * @param OP The operator to be used in the expression.
   */
  Selectable(std::string_view &&Field, std::string_view &&TblName,
             std::string_view &&OP)
      : Expr(std::string(OP) + "(" + std::string(Field) + ")"),
        TblName(TblName) {}

  /// Returns the string representation of the expression.
  inline std::string toString() const { return Expr; }

  /// Returns the table name associated with the expression.
  inline std::string tableName() const { return TblName; }

  /// The return type of the expression.
  ReturnType return_type; // NOLINT

private:
  std::string Expr;    /// The string representation of the expression.
  std::string TblName; /// The table name associated with the expression.
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
   * @param Field The Field or column name.
   * @param tblName The table name.
   */
  explicit Expr(std::string_view &&Field, std::string_view &&TblName)
      : Exp(Field), TblName(TblName) {}

  /**
   * @brief Creates a new expression by applying an operator to the current
   * expression and a Value.
   *
   * @tparam Ty The type of the Value.
   * @param OP The operator to be used in the expression.
   * @param Value The Value to be used in the expression.
   *
   * @return An Expr object representing the new expression.
   */
  template <typename Ty> Expr makeExpr(std::string &&OP, Ty Value) {
    using U = std::decay_t<Ty>;
    /// If the Value is an array, a string, or a constant character pointer,
    /// wrap it in quotes and append it to the current expression.
    if constexpr (std::is_array<U>::value || std::is_same_v<U, std::string> ||
                  std::is_same_v<U, const char *>) {
      return Expr(Exp + " " + OP + " " + "'" + Value + "'", TblName);
    }
    /// If the Value is another Expr object, append it to the current
    /// expression.
    else if constexpr (std::is_same_v<Ty, Expr>) {
      return Expr(Exp + " " + OP + " " + Value.Exp, TblName);
    }
    /// Otherwise, convert the Value to a string and append it to the current
    /// expression.
    else {
      return Expr(Exp + " " + OP + " " + std::to_string(Value), TblName);
    }
  }

  template <typename Ty> inline Expr operator==(Ty Val) {
    return Expr(makeExpr("=", Val));
  }
  template <typename Ty> inline Expr operator=(Ty Val) { // NOLINT
    return Expr(makeExpr("=", Val));
  }
  template <typename Ty> inline Expr operator|(Ty Val) {
    return Expr(makeExpr(",", Val));
  }
  template <typename Ty> inline Expr operator!=(Ty Val) {
    return Expr(makeExpr("!=", Val));
  }
  template <typename Ty> inline Expr operator<(Ty Val) {
    return Expr(makeExpr("<", Val));
  }
  template <typename Ty> inline Expr operator>(Ty Val) {
    return Expr(makeExpr(">", Val));
  }
  template <typename Ty> inline Expr operator<=(Ty Val) {
    return Expr(makeExpr("<=", Val));
  }
  template <typename Ty> inline Expr operator>=(Ty Val) {
    return Expr(makeExpr(">=", Val));
  }
  inline Expr operator%(std::string &&Val) {
    return Expr(makeExpr("like", Val));
  }
  inline Expr operator^(std::string &&Val) {
    return Expr(makeExpr("not like", Val));
  }
  inline Expr operator&&(Expr &&Val) { return Expr(makeExpr("and", Val)); }
  inline Expr operator||(Expr &&Val) { return Expr(makeExpr("or", Val)); }

  inline std::string toString() const { return Exp; }
  inline std::string tableName() const { return TblName; }
  inline std::string print() const { return TblName + ", " + Exp; }

private:
  std::string Exp;     /// The string representation of the expression.
  std::string TblName; /// The table name associated with the expression.
};

template <typename Ty, typename ID> class QueryWrapper {
public:
  QueryWrapper(PGconn *Conn, std::string_view TableName)
      : Conn(Conn), TableName(TableName) {}

  QueryWrapper(PGconn *Conn, std::string_view TableName, Ty &QueryResult,
               const std::string &SelectSql, const std::string &WhereSql,
               const std::string &GroupBySql, const std::string &HavingSql,
               const std::string &OrderBySql, const std::string &LimitSql,
               const std::string &OffsetSql)
      : Conn(Conn), TableName(TableName), QueryResult(QueryResult),
        SelectSql(SelectSql), WhereSql(WhereSql), GroupBySql(GroupBySql),
        HavingSql(HavingSql), OrderBySql(OrderBySql), LimitSql(LimitSql),
        OffsetSql(OffsetSql) {}

  template <typename... ArgTys> inline auto select(ArgTys &&...Args) {
    std::string Sql = "select ";
    if constexpr (sizeof...(ArgTys) > 0) {
      selectImpl(Sql, std::forward<ArgTys>(Args)...);
    } else {
      Sql += " * ";
    }
    Sql += " from " + TableName;
    (*this).SelectSql = Sql;
    return newQuery(std::tuple<decltype(Args.return_type)...>{});
  }

  inline QueryWrapper &&set(const Expr &Expr) {
    TableName = Expr.tableName();
    (*this).SetSql = " set " + Expr.toString();
    return std::move(*this);
  }
  inline QueryWrapper &&where(const Expr &Expr) {
    TableName = Expr.tableName();
    (*this).WhereSql = " where (" + Expr.toString() + ")";
    return std::move(*this);
  }
  inline QueryWrapper &&where(ID Id) {
    std::stringstream SS;
    SS << " where (" << getAutoKey<Ty>() << " = " << Id << ")";
    (*this).WhereSql = SS.str();
    return std::move(*this);
  }
  inline QueryWrapper &&groupBy(const Expr &Expr) {
    (*this).GroupBySql = " group by (" + Expr.toString() + ")";
    return std::move(*this);
  }
  inline QueryWrapper &&having(const Expr &Expr) {
    (*this).HavingSql = " having (" + Expr.toString() + ")";
    return std::move(*this);
  }
  inline QueryWrapper &&orderBy(const Expr &Expr) {
    (*this).OrderBySql = " order by " + Expr.toString() + " asc";
    return std::move(*this);
  }
  inline QueryWrapper &&orderByDesc(const Expr &Expr) {
    (*this).OrderBySql = " order by " + Expr.toString() + " desc";
    return std::move(*this);
  }
  inline QueryWrapper &&limit(std::size_t N) {
    (*this).LimitSql = " limit " + std::to_string(N);
    return std::move(*this);
  }
  inline QueryWrapper &&offset(std::size_t N) {
    (*this).OffsetSql = " offset " + std::to_string(N);
    return std::move(*this);
  }

  std::string toString() {
    if (SelectSql.empty())
      SelectSql = "select * from " + TableName;

    return SelectSql + WhereSql + GroupBySql + HavingSql + OrderBySql +
           LimitSql + OffsetSql + ";";
  }

  std::vector<Ty> toVector() { return execute<Ty>(toString()); }

private:
  template <typename _Ty>
  constexpr std::enable_if_t<is_reflection<_Ty>::value, std::vector<_Ty>>
  execute(const std::string &Sql) {
    std::vector<_Ty> RetVector;
    LOG_DEBUG << "query: " << Sql;
    Res = PQexec(Conn, Sql.c_str());
    if (PQresultStatus(Res) != PGRES_TUPLES_OK) {
      LOG_ERROR << PQresultErrorMessage(Res);
      PQclear(Res);
      return RetVector;
    }
    size_t Ntuples = PQntuples(Res);
    for (size_t I = 0; I < Ntuples; I++) {
      _Ty Tp = {};
      forEach(Tp, [this, &Tp, &I](auto Item, auto Field, auto J) {
        this->assignValue(Tp.*Item, I, static_cast<int>(decltype(J)::value));
      });
      RetVector.push_back(std::move(Tp));
    }
    PQclear(Res);
    return RetVector;
  }

  template <typename _Ty>
  constexpr std::enable_if_t<!is_reflection<_Ty>::value, std::vector<_Ty>>
  execute(const std::string &Sql) {
    std::vector<_Ty> RetVector;
    LOG_DEBUG << "query: " << Sql;
    Res = PQexec(Conn, Sql.c_str());
    if (PQresultStatus(Res) != PGRES_TUPLES_OK) {
      LOG_ERROR << PQresultErrorMessage(Res);
      PQclear(Res);
      return RetVector;
    }
    size_t Ntuples = PQntuples(Res);
    for (size_t I = 0; I < Ntuples; I++) {
      _Ty Tp = {};
      int Index = 0;
      forEach(Tp, [this, &I, &Index](auto &Item, auto J) {
        if constexpr (is_reflection_v<std::decay_t<decltype(Item)>>) {
          std::decay_t<decltype(Item)> T = {};
          forEach(T, [this, &Index, &T](auto Elem, auto Field, auto J) {
            assignValue(T.*Elem, J, Index++);
          });
          Item = std::move(T);
        } else {
          assignValue(Item, static_cast<int>(I), Index++);
        }
      });
      RetVector.push_back(Tp);
    }
    PQclear(Res);
    return RetVector;
  }

  template <typename... ArgTys>
  inline QueryWrapper<std::tuple<ArgTys...>, ID>
  newQuery(std::tuple<ArgTys...> &&QueryRes) {
    return QueryWrapper<std::tuple<ArgTys...>, ID>(
        Conn, TableName, QueryRes, SelectSql, WhereSql, GroupBySql, HavingSql,
        OrderBySql, LimitSql, OffsetSql);
  }

  template <typename... ArgTys>
  inline void selectImpl(std::string &Sql, ArgTys &&...Args) {
    constexpr auto Size = sizeof...(ArgTys);
    auto Tp = std::make_tuple(std::forward<ArgTys>(Args)...);
    forEach(Tp, [&](auto Arg, auto I) {
      if constexpr (std::is_same_v<decltype(Arg),
                                   Selectable<decltype(Arg.return_type)>>) {
        bool SameTable = Arg.tableName() == TableName;
        assert(SameTable);
        (void)SameTable;
        TableName = Arg.tableName();
        Sql += Arg.toString();
        if (I != Size - 1) {
          Sql += ", ";
        }
      }
    });
  }

  template <typename _Ty>
  constexpr void assignValue(_Ty &&Value, int Row, int Col) {
    using U = std::remove_const_t<std::remove_reference_t<_Ty>>;
    if constexpr (std::is_integral<U>::value &&
                  !(std::is_same<U, int64_t>::value ||
                    std::is_same<U, uint64_t>::value)) {
      Value = atoi(PQgetvalue(Res, Row, Col));
    } else if constexpr (std::is_enum_v<U>) {
      Value = static_cast<U>(atoi(PQgetvalue(Res, Row, Col)));
    } else if constexpr (std::is_floating_point<U>::value) {
      Value = atof(PQgetvalue(Res, Row, Col));
    } else if constexpr (std::is_same<U, int64_t>::value ||
                         std::is_same<U, uint64_t>::value) {
      Value = atoll(PQgetvalue(Res, Row, Col));
    } else if constexpr (std::is_same<U, std::string>::value) {
      Value = PQgetvalue(Res, Row, Col);
    } else if constexpr (std::is_array<U>::value &&
                         std::is_same<char, std::remove_pointer_t<
                                                std::decay_t<U>>>::value) {
      auto *Ptr = PQgetvalue(Res, Row, Col);
      memcpy(Value, Ptr, sizeof(U));
    } else {
      LOG_ERROR << "unsupported type:" << std::is_array<U>::value;
    }
  }

  PGconn *Conn;
  PGresult *Res;

  std::string TableName;

  Ty QueryResult;

  std::string SelectSql;
  std::string WhereSql;
  std::string GroupBySql;
  std::string HavingSql;
  std::string OrderBySql;
  std::string LimitSql;
  std::string OffsetSql;
};

template <typename Ty, typename ID> class UpdateWrapper {
public:
  UpdateWrapper(PGconn *Conn, std::string_view TableName,
                const std::string &UpdateSql = std::string("update"))
      : Conn(Conn), TableName(TableName),
        UpdateSql(UpdateSql + " " + std::string(TableName)) {}

  UpdateWrapper(PGconn *Conn, std::string_view TableName,
                const std::string &UpdateSql, const std::string &SetSql,
                const std::string &WhereSql)
      : Conn(Conn), TableName(TableName), UpdateSql(UpdateSql), SetSql(SetSql),
        WhereSql(WhereSql) {}

  inline UpdateWrapper &&set(const Expr &Expr) {
    TableName = Expr.tableName();
    (*this).SetSql = " set " + Expr.toString();
    return std::move(*this);
  }

  inline UpdateWrapper &&set(Ty &&T) {
    std::string Sql;
    size_t Size = getValue<Ty>();
    forEach(T, [&](auto &Item, auto Field, auto J) {
      Sql += std::string(Field) + " = $" + std::to_string(++Idx);
      if (J != Size - 1) {
        Sql += ", ";
      }
      detail::setParamValue(ParamValues, T.*Item);
    });
    (*this).SetSql = " set " + Sql;
    return std::move(*this);
  }

  inline UpdateWrapper &&where(const Expr &Expr) {
    TableName = Expr.tableName();
    (*this).WhereSql = " where (" + Expr.toString() + ")";
    return std::move(*this);
  }

  inline UpdateWrapper &&where(ID Id) {
    detail::setParamValue(ParamValues, Id);
    (*this).WhereSql = " where (" + std::string(getAutoKey<Ty>()) + " = $" +
                       std::to_string(++Idx) + ")";
    return std::move(*this);
  }

  bool execute() {
    std::string Sql = toString();
    LOG_TRACE << "update prepare: " << Sql;
    if (!this->prepare(Sql))
      return false;

    return updateImpl(Sql);
  }

  std::string toString() { return UpdateSql + SetSql + WhereSql + ";"; }

private:
  bool prepare(const std::string &Sql) {
    Res = PQprepare(Conn, "", Sql.data(), static_cast<int>(getValue<Ty>()),
                    nullptr);
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(Conn);
      return false;
    }
    PQclear(Res);
    return true;
  }

  bool updateImpl(std::string &Sql) {
    if (ParamValues.empty())
      return false;

    std::vector<const char *> ParamValuesBuf;
    ParamValuesBuf.reserve(ParamValues.size());
    for (auto &Item : ParamValues)
      ParamValuesBuf.push_back(Item.data());

    // For debug
    std::stringstream SS;
    SS << "params: ";
    for (size_t I = 0; I < ParamValuesBuf.size(); I++)
      SS << (I + 1) << " = " << *(ParamValuesBuf.data() + I) << ", ";
    LOG_DEBUG << SS.str();

    Res = PQexecPrepared(Conn, "", static_cast<int>(ParamValues.size()),
                         ParamValuesBuf.data(), nullptr, nullptr, 0);
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQresultErrorMessage(Res);
      PQclear(Res);
      return false;
    }
    PQclear(Res);
    return true;
  }

  PGconn *Conn;
  PGresult *Res;

  std::string TableName;
  std::string UpdateSql;
  std::string WhereSql;
  std::string SetSql;

  std::vector<std::vector<char>> ParamValues;
  size_t Idx = 0;
};

template <typename Ty, typename ID> class DeleteWrapper {
public:
  DeleteWrapper(PGconn *Conn, std::string_view TableName,
                const std::string &DeleteSql = std::string("delete"))
      : Conn(Conn), TableName(TableName),
        DeleteSql(DeleteSql + " from " + std::string(TableName)) {}

  DeleteWrapper(PGconn *Conn, std::string_view TableName,
                const std::string &DeleteSql, const std::string &WhereSql)
      : Conn(Conn), TableName(TableName), DeleteSql(DeleteSql),
        WhereSql(WhereSql) {}

  inline DeleteWrapper &&where(const Expr &Expr) {
    TableName = Expr.tableName();
    (*this).WhereSql = " where (" + Expr.toString() + ")";
    return std::move(*this);
  }

  inline DeleteWrapper &&where(ID Id) {
    detail::setParamValue(ParamValues, Id);
    (*this).WhereSql = " where (" + std::string(getAutoKey<Ty>()) + " = $" +
                       std::to_string(++Idx) + ")";
    return std::move(*this);
  }

  bool execute() {
    std::string Sql = toString();
    LOG_TRACE << "delete prepare: " << Sql;
    if (!this->prepare(Sql))
      return false;

    return deleteImpl(Sql);
  }

  std::string toString() { return DeleteSql + WhereSql + ";"; }

private:
  bool prepare(const std::string &Sql) {
    Res = PQprepare(Conn, "", Sql.data(), static_cast<int>(getValue<Ty>()),
                    nullptr);
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(Conn);
      return false;
    }
    PQclear(Res);
    return true;
  }

  bool deleteImpl(std::string &Sql) {
    if (ParamValues.empty())
      return false;

    std::vector<const char *> ParamValuesBuf;
    ParamValuesBuf.reserve(ParamValues.size());
    for (auto &Item : ParamValues)
      ParamValuesBuf.push_back(Item.data());

    // For debug
    std::stringstream SS;
    SS << "params: ";
    for (size_t I = 0; I < ParamValuesBuf.size(); I++)
      SS << (I + 1) << " = " << *(ParamValuesBuf.data() + I) << ", ";
    LOG_DEBUG << SS.str();

    Res = PQexecPrepared(Conn, "", static_cast<int>(ParamValues.size()),
                         ParamValuesBuf.data(), nullptr, nullptr, 0);
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQresultErrorMessage(Res);
      PQclear(Res);
      return false;
    }
    PQclear(Res);
    return true;
  }

  PGconn *Conn;
  PGresult *Res;

  std::string TableName;
  std::string DeleteSql;
  std::string WhereSql;

  std::vector<std::vector<char>> ParamValues;
  size_t Idx = 0;
};

template <typename Ty> class InsertWrapper {
public:
  InsertWrapper(PGconn *Conn, std::string_view TableName)
      : Conn(Conn), TableName(TableName) {}

  int insert(Ty &T) {
    std::string Sql = generateInsertSql();
    LOG_TRACE << " insert prepare: " << Sql;
    if (!prepare(Sql))
      return 0;

    return insertImpl(Sql, T);
  }

  int insert(std::vector<Ty> &T) {
    std::string Sql = generateInsertSql();
    LOG_TRACE << " insert prepare: " << Sql;
    if (!prepare(Sql))
      return 0;

    for (auto &Item : T) {
      if (!insertImpl(Sql, Item)) {
        execute("rollback;");
        return 0;
      }
    }

    if (!execute("commit;"))
      return 0;

    return T.size();
  }

private:
  bool execute(const std::string &Sql) {
    LOG_DEBUG << "exec: " << Sql;
    Res = PQexec(Conn, Sql.data());
    bool Ret = PQresultStatus(Res) == PGRES_COMMAND_OK;
    PQclear(Res);
    return Ret;
  }

  bool insertImpl(std::string &Sql, Ty &T) {
    std::vector<std::vector<char>> ParamValues;
    forEach(T, [&](auto &Item, auto Field, auto J) {
      if (!isAutoKey<Ty>(getName<Ty>(J).data())) {
        detail::setParamValue(ParamValues, T.*Item);
      }
    });
    if (ParamValues.empty())
      return false;

    std::vector<const char *> ParamValuesBuf;
    ParamValuesBuf.reserve(ParamValues.size());
    for (auto &Item : ParamValues)
      ParamValuesBuf.push_back(Item.data());

    // For debugging
    std::stringstream SS;
    SS << "params: ";
    for (size_t I = 0; I < ParamValuesBuf.size(); I++)
      SS << I << "=" << *(ParamValuesBuf.data() + I) << ", ";
    LOG_DEBUG << SS.str();

    Res = PQexecPrepared(Conn, "", static_cast<int>(ParamValues.size()),
                         ParamValuesBuf.data(), nullptr, nullptr, 0);
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQresultErrorMessage(Res);
      PQclear(Res);
      return false;
    }
    PQclear(Res);
    return true;
  }

  constexpr auto generateInsertSql() {
    std::string TblName = getName<Ty>().data();
    std::string Sql = "insert into " + TblName + "(";

    auto FieldNames = getArray<Ty>();
    constexpr auto FieldSize = getValue<Ty>();
    for (size_t I = 0; I < FieldSize; I++) {
      std::string FieldName = FieldNames[I].data();
      /// Skip if is auto key
      if (isAutoKey<Ty>(FieldName))
        continue;

      Sql += FieldName;
      if (I != FieldSize - 1)
        Sql += ", ";
    }
    Sql += ") values(";

    int Idx = 0;
    for (size_t I = 0; I < FieldSize; I++) {
      std::string FieldName = getName<Ty>(I).data();
      /// Skip if is auto key
      if (isAutoKey<Ty>(FieldName))
        continue;

      Sql += "$" + std::to_string(++Idx);
      if (I != FieldSize - 1)
        Sql += ", ";
    }
    Sql += ");";
    return Sql;
  }

  bool prepare(const std::string &Sql) {
    Res = PQprepare(Conn, "", Sql.data(), static_cast<int>(getValue<Ty>()),
                    nullptr);
    if (PQresultStatus(Res) != PGRES_COMMAND_OK) {
      LOG_ERROR << PQerrorMessage(Conn);
      return false;
    }
    PQclear(Res);
    return true;
  }

  PGconn *Conn;
  PGresult *Res;

  std::string TableName;
};

template <typename Ty> struct FieldAttribute;

template <typename Ty, typename U> struct FieldAttribute<U Ty::*> {
  using type = Ty;
  using return_type = U;
};

template <typename U>
constexpr std::string_view getFieldName(std::string_view FullName) {
  using Ty = typename FieldAttribute<U>::type;
  return FullName.substr(getName<Ty>().length() + 2, FullName.length());
}

template <typename U>
constexpr std::string_view getTableName(std::string_view FullName) {
  using Ty = typename FieldAttribute<U>::type;
  return getName<Ty>();
}

#define VALUE(Field)                                                           \
  lynx::Expr(lynx::getFieldName<decltype(&(Field))>(std::string_view(#Field)), \
             lynx::getTableName<decltype(&(Field))>(std::string_view(#Field)))

#define ORM_AGG(Field, OP, type)                                               \
  lynx::Selectable<type>(                                                      \
      lynx::getFieldName<decltype(&(Field))>(std::string_view(#Field)),        \
      lynx::getTableName<decltype(&(Field))>(std::string_view(#Field)), OP)

#define FIELD(Field)                                                           \
  ORM_AGG(Field, "", lynx::FieldAttribute<decltype(&(Field))>::return_type)

#define ORM_COUNT(Field) ORM_AGG(Field, "count", std::size_t)
#define ORM_SUM(Field)                                                         \
  ORM_AGG(Field, "sum", lynx::FieldAttribute<decltype(&(Field))>::return_type)
#define ORM_AVG(Field)                                                         \
  ORM_AGG(Field, "avg", lynx::FieldAttribute<decltype(&(Field))>::return_type)
#define ORM_MAX(Field)                                                         \
  ORM_AGG(Field, "max", lynx::FieldAttribute<decltype(&(Field))>::return_type)
#define ORM_MIN(Field)                                                         \
  ORM_AGG(Field, "min", lynx::FieldAttribute<decltype(&(Field))>::return_type)

} // namespace lynx

#endif
