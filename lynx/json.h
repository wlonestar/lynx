#ifndef LYNX_JSON_H
#define LYNX_JSON_H

#include "lynx/reflection.h"

#include <nlohmann/json.hpp>

namespace lynx {

using json = nlohmann::json;

#define TO_JSON(T)                                                             \
  void to_json(lynx::json &j, const T &t) {                                    \
    lynx::forEach(                                                             \
        t, [&t, &j](auto item, auto field, auto i) { j[field] = t.*item; });   \
  }

#define FROM_JSON(T)                                                           \
  /* NOLINTNEXTLINE */                                                         \
  void from_json(const lynx::json &j, T &t) {                                  \
    lynx::forEach(t, [&t, &j](auto item, auto field, auto i) {                 \
      j.at(field).get_to(t.*item);                                             \
    });                                                                        \
  }

#define JSON_SERIALIZE(T)                                                      \
  TO_JSON(T);                                                                  \
  FROM_JSON(T);

} // namespace lynx

#endif
