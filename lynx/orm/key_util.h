#ifndef LYNX_ORM_KEY_UTIL_H
#define LYNX_ORM_KEY_UTIL_H

#include "lynx/reflection.h"

#include <set>
#include <string_view>
#include <unordered_map>

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

inline std::unordered_map<std::string_view, std::string_view>
    g_orm_auto_key_map;

inline int addAutoKeyField(std::string_view key, std::string_view value) {
  g_orm_auto_key_map.emplace(key, value);
  return 0;
}

template <typename T> inline auto getAutoKey() {
  using U = decltype(reflectMembersFunc(std::declval<T>()));
  auto it = g_orm_auto_key_map.find(U::struct_name());
  return it == g_orm_auto_key_map.end() ? "" : it->second;
}

template <typename T> inline auto isAutoKey(std::string_view fieldName) {
  using U = decltype(reflectMembersFunc(std::declval<T>()));
  auto it = g_orm_auto_key_map.find(U::struct_name());
  return it == g_orm_auto_key_map.end() ? false : it->second == fieldName;
}

#define REGISTER_AUTO_KEY(class_name, key)                                     \
  inline auto MACRO_CONCAT(class_name, __LINE__) =                             \
      lynx::addAutoKeyField(#class_name, #key);

} // namespace lynx

#endif
