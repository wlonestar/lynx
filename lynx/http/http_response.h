#ifndef LYNX_HTTP_HTTP_RESPONSE_H
#define LYNX_HTTP_HTTP_RESPONSE_H

#include "lynx/http/http_status.h"

#include <map>
#include <string>

namespace lynx {

class Buffer;

/**
 * @class HttpResponse
 * @brief Represents an HTTP response.
 *
 * This class encapsulates the information in an HTTP response, including the
 * response status code, headers, and body. It provides methods for setting and
 * getting these attributes, as well as methods for adding headers.
 */
class HttpResponse {
public:
  /**
   * @brief Constructs a new HttpResponse.
   *
   * @param close A boolean indicating whether the connection should be closed
   * after the response is sent.
   */
  explicit HttpResponse(bool close) : close_connection_(close) {}

  void setStatusCode(HttpStatus status) { status_ = status; }
  void setCloseConnection(bool on) { close_connection_ = on; }

  void setBody(const std::string &body) { body_ = body; }
  void setContentType(const std::string &contentType) {
    addHeader("Content-Type", contentType);
  }

  bool closeConnection() const { return close_connection_; }

  void addHeader(const std::string &key, const std::string &value) {
    headers_[key] = value;
  }

  void appendToBuffer(Buffer *output) const;

private:
  std::map<std::string, std::string> headers_;
  HttpStatus status_{};
  bool close_connection_;
  std::string body_;
};

} // namespace lynx

#endif
