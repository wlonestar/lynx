#ifndef LYNX_HTTP_HTTP_RESPONSE_H
#define LYNX_HTTP_HTTP_RESPONSE_H


#include <map>
#include <string>

namespace lynx {

class Buffer;

class HttpResponse  {
public:
  enum HttpStatusCode {
    UNKNOWN,
    Ok200 = 200,
    MovedPermanently301 = 301,
    BadRequest400 = 400,
    NotFound404 = 404,
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

  void addHeader(const std::string &key, const std::string &value) {
    headers_[key] = value;
  }

  void setBody(const std::string &body) { body_ = body; }

  void appendToBuffer(Buffer *output) const;

private:
  std::map<std::string, std::string> headers_;
  HttpStatusCode status_code_{};
  std::string status_message_;
  bool close_connection_;
  std::string body_;
};

} // namespace lynx

#endif
