#ifndef LYNX_ORM_SQL_UTIL_H
#define LYNX_ORM_SQL_UTIL_H

#include "lynx/orm/key_util.h"
#include "lynx/orm/reflection.h"
#include "lynx/orm/traits_util.h"

namespace lynx {

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

template <typename T>
constexpr void setParamValue(std::vector<std::vector<char>> &paramValues,
                             T &&value) {
  using U = std::remove_const_t<std::remove_reference_t<T>>;
  if constexpr (std::is_same_v<U, int64_t> ||
                std::is_same_v<U, uint64_t>) { /// 64 bit integer type
    std::vector<char> temp(65, 0);
    auto v_str = std::to_string(value);
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(temp);
  } else if constexpr (std::is_integral_v<U> ||
                       std::is_floating_point_v<U>) { /// floating type
    std::vector<char> temp(20, 0);
    auto v_str = std::to_string(value);
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(std::move(temp));
  } else if constexpr (std::is_enum_v<U>) { /// enum type
    std::vector<char> temp(20, 0);
    auto v_str = std::to_string(static_cast<std::underlying_type_t<U>>(value));
    memcpy(temp.data(), v_str.data(), v_str.size());
    paramValues.push_back(std::move(temp));
  } else if constexpr (std::is_same_v<U, std::string>) { /// std::string type
    std::vector<char> temp = {};
    std::copy(value.data(), value.data() + value.size() + 1,
              std::back_inserter(temp));
    paramValues.push_back(std::move(temp));
  } else if constexpr (std::is_array<U>::value) { /// array type
    std::vector<char> temp = {};
    std::copy(value, value + ArraySize<U>::value, std::back_inserter(temp));
    paramValues.push_back(std::move(temp));
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

// template <typename T> constexpr auto generateInsertSql() {
//   std::string table_name = getName<T>().data();
//   std::string sql = "insert into " + table_name + "(";

//   auto field_names = getArray<T>();
//   constexpr auto field_size = getValue<T>();
//   for (size_t i = 0; i < field_size; i++) {
//     std::string field_name = field_names[i].data();
//     /// Skip if is auto key
//     if (isAutoKey<T>(field_name)) {
//       continue;
//     }
//     sql += field_name;
//     if (i != field_size - 1) {
//       sql += ", ";
//     }
//   }
//   sql += ") values(";

//   int idx = 0;
//   for (size_t i = 0; i < field_size; i++) {
//     std::string field_name = getName<T>(i).data();
//     /// Skip if is auto key
//     if (isAutoKey<T>(field_name)) {
//       continue;
//     }
//     sql += "$" + std::to_string(++idx);
//     if (i != field_size - 1) {
//       sql += ", ";
//     }
//   }
//   sql += ");";
//   return sql;
// }

} // namespace lynx

#endif
