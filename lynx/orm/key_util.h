#ifndef LYNX_ORM_KEY_UTIL_H
#define LYNX_ORM_KEY_UTIL_H

#include "lynx/orm/reflection.h"

#include <set>
#include <string_view>
#include <unordered_map>

namespace lynx {

/**
 * @brief Struct representing the mapping of keys to fields in ORM classes.
 */
struct KeyMap {
  std::string fields; // NOLINT
};

/**
 * @brief Struct representing the mapping of auto-increment keys to fields in
 * ORM classes.
 */
struct AutoKeyMap {
  std::string fields; // NOLINT
};

/**
 * @brief Struct representing the mapping of non-null fields to fields in ORM
 * classes.
 */
struct NotNullMap {
  std::set<std::string> fields; // NOLINT
};

/**
 * @brief Map containing the auto-increment key fields for all ORM classes.
 */
inline std::unordered_map<std::string_view, std::string_view>
    g_orm_auto_key_map;

/**
 * @brief Adds an auto-increment key field to the map.
 *
 * @param key The key for the field.
 * @param value The value for the field.
 *
 * @return int Always returns 0.
 */
inline int addAutoKeyField(std::string_view key, std::string_view value) {
  g_orm_auto_key_map.emplace(key, value);
  return 0;
}

/**
 * @brief Gets the auto-increment key field for a given ORM class.
 *
 * @tparam T The ORM class type.
 *
 * @return The auto-increment key field for the class, or an empty string if it
 * does not exist.
 */
template <typename T> inline auto getAutoKey() {
  using U = decltype(reflectMembersFunc(std::declval<T>()));
  auto it = g_orm_auto_key_map.find(U::struct_name());
  return it == g_orm_auto_key_map.end() ? "" : it->second;
}

/**
 * @brief Checks if a field is an auto-increment key field for a given ORM
 * class.
 *
 * @tparam T The ORM class type.
 * @param fieldName The name of the field.
 *
 * @return `true` if the field is an auto-increment key field, `false`
 * otherwise.
 */
template <typename T> inline auto isAutoKey(std::string_view fieldName) {
  using U = decltype(reflectMembersFunc(std::declval<T>()));
  auto it = g_orm_auto_key_map.find(U::struct_name());
  return it == g_orm_auto_key_map.end() ? false : it->second == fieldName;
}

/**
 * @brief Macro for registering an auto-increment key field for an ORM class.
 *
 * @param class_name The name of the ORM class.
 * @param key The name of the key field.
 */
#define REGISTER_AUTO_KEY(class_name, key)                                     \
  inline auto MACRO_CONCAT(class_name, __LINE__) =                             \
      lynx::addAutoKeyField(#class_name, #key);

} // namespace lynx

#endif
