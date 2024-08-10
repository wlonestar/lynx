#ifndef LYNX_WEB_COMMON_RESULT_H
#define LYNX_WEB_COMMON_RESULT_H

#include "lynx/orm/json.h"

namespace lynx {

template <typename T> struct Result {
  int status;          // NOLINT
  std::string message; // NOLINT
  T data;              // NOLINT
};

template <typename T> // NOLINTNEXTLINE
void to_json(json &j, const Result<T> &result) {
  j["status"] = result.status;
  j["message"] = result.message;
  j["data"] = result.data;
}

template <typename T> // NOLINTNEXTLINE
void from_json(const json &j, Result<T> &result) {
  j.at("status").get_to(result.status);
  j.at("message").get_to(result.message);
  j.at("data").get_to(result.data);
}

template <typename T>
Result<T> makeOkResult(const std::string &message, const T &data) {
  return lynx::Result<T>{200, message, data};
}

Result<std::string> makeErrorResult(const std::string &message,
                                    const std::string &hint) {
  return lynx::Result<std::string>{400, message, hint};
}

} // namespace lynx

#endif
