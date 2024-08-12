#ifndef LYNX_HTTP_HTTP_REQUEST_H
#define LYNX_HTTP_HTTP_REQUEST_H

#include "lynx/base/timestamp.h"
#include "lynx/http/http_status.h"

#include <cassert>
#include <map>
#include <strings.h>

namespace lynx {

#define HTTP_METHOD_MAP(XX)                                                    \
  XX(0, DELETE, DELETE)                                                        \
  XX(1, GET, GET)                                                              \
  XX(2, HEAD, HEAD)                                                            \
  XX(3, POST, POST)                                                            \
  XX(4, PUT, PUT)

enum class HttpMethod {
#define XX(num, name, string) name = (num),
  HTTP_METHOD_MAP(XX)
#undef XX
      INVALID_METHOD
};

HttpMethod stringToHttpMethod(const std::string &m);
HttpMethod charsToHttpMethod(const char *m);
const char *methodToString(const HttpMethod &m);

struct CaseInsensitiveLess {
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

class HttpRequest {
public:
  using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

  HttpRequest(uint8_t version = 0x11, bool close = true);

  bool isClose() const { return close_; }
  void setClose(bool c) { close_ = c; }

  bool isWebsocket() const { return websocket_; }
  void setWebsocket(bool w) { websocket_ = w; }

  HttpMethod method() const { return method_; }
  void setMethod(HttpMethod m) { method_ = m; }

  uint8_t version() const { return version_; }
  void setVersion(uint8_t v) { version_ = v; }

  const std::string &path() const { return path_; }
  void setPath(const std::string &p) { path_ = p; }

  const std::string &query() const { return query_; }
  void setQuery(const std::string &q) { query_ = q; }

  const std::string &uri() const { return uri_; }
  void setUri(const std::string &u) { uri_ = u; }

  void setFragment(const std::string &f) { fragment_ = f; }

  const std::string &body() const { return body_; }
  void setBody(const std::string &b) { body_ = b; }

  const MapType &headers() const { return headers_; }
  void setHeaders(const MapType &h) { headers_ = h; }

  const MapType &params() const { return params_; }
  void setParams(const MapType &p) { params_ = p; }

  const MapType &cookies() const { return cookies_; }
  void setCookies(const MapType &c) { cookies_ = c; }

  std::string getHeader(const std::string &key,
                        const std::string &def = "") const;
  std::string getParam(const std::string &key, const std::string &def = "");
  std::string getCookie(const std::string &key, const std::string &def = "");

  void setHeader(const std::string &key, const std::string &val);
  void setParam(const std::string &key, const std::string &val);
  void setCookie(const std::string &key, const std::string &val);

  void delHeader(const std::string &key);
  void delParam(const std::string &key);
  void delCookie(const std::string &key);

  bool hasHeader(const std::string &key, std::string *val = nullptr);
  bool hasParam(const std::string &key, std::string *val = nullptr);
  bool hasCookie(const std::string &key, std::string *val = nullptr);

  std::ostream &dump(std::ostream &os) const;

  std::string toString() const;

  void initQueryParam();
  void initBodyParam();
  void initCookies();

  void swap(HttpRequest &other) {
    std::swap(method_, other.method_);
    std::swap(version_, other.version_);
    std::swap(close_, other.close_);
    std::swap(websocket_, other.websocket_);
    std::swap(parser_param_flag_, other.parser_param_flag_);
    path_.swap(other.path_);
    query_.swap(other.query_);
    fragment_.swap(other.fragment_);
    body_.swap(other.body_);
    headers_.swap(other.headers_);
    params_.swap(other.params_);
    cookies_.swap(other.cookies_);
  }

private:
  HttpMethod method_;
  uint8_t version_;
  bool close_;
  bool websocket_;

  uint8_t parser_param_flag_;

  std::string path_;
  std::string query_;
  std::string uri_;
  std::string fragment_;
  std::string body_;
  MapType headers_;
  MapType params_;
  MapType cookies_;
};

} // namespace lynx

#endif
