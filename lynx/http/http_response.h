#ifndef LYNX_HTTP_HTTP_RESPONSE_H
#define LYNX_HTTP_HTTP_RESPONSE_H

#include "lynx/http/http_status.h"

#include <map>
#include <string>

namespace lynx {

class Buffer;

class HttpResponse {
public:
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
