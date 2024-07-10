#ifndef LYNX_HTTP_HTTP_RESPONSE_H
#define LYNX_HTTP_HTTP_RESPONSE_H

#include "lynx/base/copyable.h"

#include <map>
#include <string>

namespace lynx {

class Buffer;

class HttpResponse : public Copyable {
public:
  enum HttpStatusCode {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  explicit HttpResponse(bool close) : close_connection_(close) {}

  void setStatusCode(HttpStatusCode code) { status_code_ = code; }

  void setStatusMessage(const std::string &message) {
    status_message_ = message;
  }

  void setCloseConnection(bool on) { close_connection_ = on; }

  bool closeConnection() const { return close_connection_; }

  void setContentType(const std::string &contentType) {
    addHeader("Content-Type", contentType);
  }

  // FIXME: replace string with StringPiece
  void addHeader(const std::string &key, const std::string &value) {
    headers_[key] = value;
  }

  void setBody(const std::string &body) { body_ = body; }

  void appendToBuffer(Buffer *output) const;

private:
  std::map<std::string, std::string> headers_;
  HttpStatusCode status_code_{};
  // FIXME: add http version
  std::string status_message_;
  bool close_connection_;
  std::string body_;
};

} // namespace lynx

#endif
