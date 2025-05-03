#ifndef LYNX_WEB_COMMON_RESULT_H
#define LYNX_WEB_COMMON_RESULT_H

#include "lynx/orm/json.h"

namespace lynx {

/**
 * @struct Result
 * @brief Represents a result with a status code, a message, and optional data.
 *
 * This struct is used to represent the result of an operation, including the
 * status code, a message, and any additional data.
 *
 * @tparam T The type of the data associated with the result.
 */
template <typename T> struct Result {
  int status;          /// NOLINT. The status code of the result.
  std::string message; /// NOLINT. The message associated with the result.
  T data;              /// NOLINT. The data associated with the result.
};

/**
 * @brief Serializes a Result object to a JSON object.
 *
 * @tparam T The type of the data associated with the result.
 * @param j The JSON object to serialize to.
 * @param result The Result object to serialize.
 */
template <typename T> // NOLINTNEXTLINE
void to_json(json &j, const Result<T> &result) {
  j["status"] = result.status;
  j["message"] = result.message;
  j["data"] = result.data;
}

/**
 * @brief Deserializes a JSON object to a Result object.
 *
 * @tparam T The type of the data associated with the result.
 * @param j The JSON object to deserialize from.
 * @param result The Result object to deserialize to.
 */
template <typename T> // NOLINTNEXTLINE
void from_json(const json &j, Result<T> &result) {
  j.at("status").get_to(result.status);
  j.at("message").get_to(result.message);
  j.at("data").get_to(result.data);
}

/**
 * @brief Creates a successful Result object with the given message and data.
 *
 * @tparam T The type of the data associated with the result.
 * @param message The message to associate with the result.
 * @param data The data to associate with the result.
 * @return The created Result object.
 */
template <typename T>
Result<T> makeOkResult(const std::string &message, const T &data) {
  return lynx::Result<T>{200, message, data};
}

/**
 * @brief Creates an error Result object with the given message and hint.
 *
 * @param message The message to associate with the result.
 * @param hint A hint to provide more information about the error.
 * @return The created Result object.
 */
Result<std::string> makeErrorResult(const std::string &message,
                                    const std::string &hint) {
  return lynx::Result<std::string>{400, message, hint};
}

} // namespace lynx

#endif
