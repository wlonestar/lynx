#ifndef LYNX_HTTP_HTTP_REQUEST_H
#define LYNX_HTTP_HTTP_REQUEST_H

#include "lynx/base/timestamp.h"

#include <cassert>
#include <cstdio>
#include <map>
#include <string>

namespace lynx {

class HttpRequest  {
public:
  enum Method {
    INVALID,
    GET,
    POST,
    HEAD,
    PUT,
    DELETE,
  };

  enum Version {
    UNKNOWN,
    HTTP10,
    HTTP11,
  };

  HttpRequest() = default;

  void setVersion(Version v) { version_ = v; }
  Version getVersion() const { return version_; }

  bool setMethod(const char *start, const char *end) {
    assert(method_ == INVALID);
    std::string m(start, end);
    if (m == "GET") {
      method_ = GET;
    } else if (m == "POST") {
      method_ = POST;
    } else if (m == "HEAD") {
      method_ = HEAD;
    } else if (m == "PUT") {
      method_ = PUT;
    } else if (m == "DELETE") {
      method_ = DELETE;
    } else {
      method_ = INVALID;
    }
    return method_ != INVALID;
  }

  Method method() const { return method_; }

  const char *methodString() const {
    const char *result = "UNKNOWN";
    switch (method_) {
    case GET:
      result = "GET";
      break;
    case POST:
      result = "POST";
      break;
    case HEAD:
      result = "HEAD";
      break;
    case PUT:
      result = "PUT";
      break;
    case DELETE:
      result = "DELETE";
      break;
    default:
      break;
    }
    return result;
  }

  void setPath(const char *start, const char *end) { path_.assign(start, end); }
  const std::string &path() const { return path_; }

  void setQuery(const char *start, const char *end) {
    query_.assign(start, end);
  }
  const std::string &query() const { return query_; }

  void setReceiveTime(Timestamp t) { receive_time_ = t; }
  Timestamp receiveTime() const { return receive_time_; }

  void addHeader(const char *start, const char *colon, const char *end) {
    std::string field(start, colon);
    ++colon;
    while (colon < end && (std::isspace(*colon) != 0)) {
      ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && (std::isspace(value[value.size() - 1]) != 0)) {
      value.resize(value.size() - 1);
    }
    headers_[field] = value;
  }

  std::string getHeader(const std::string &field) const {
    std::string result;
    auto it = headers_.find(field);
    if (it != headers_.end()) {
      result = it->second;
    }
    return result;
  }

  const std::map<std::string, std::string> &headers() const { return headers_; }

  void swap(HttpRequest &that) {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receive_time_.swap(that.receive_time_);
    headers_.swap(that.headers_);
  }

private:
  Method method_{};
  Version version_{};
  std::string path_;
  std::string query_;
  Timestamp receive_time_;
  std::map<std::string, std::string> headers_;
};

} // namespace lynx

#endif
