#ifndef LYNX_HTTP_HTTP_REQUEST_H
#define LYNX_HTTP_HTTP_REQUEST_H

#include "lynx/base/timestamp.h"

#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <strings.h>
#include <tuple>

namespace lynx {

#define HTTP_METHOD_MAP(XX)                                                    \
  XX(0, DELETE, DELETE)                                                        \
  XX(1, GET, GET)                                                              \
  XX(2, HEAD, HEAD)                                                            \
  XX(3, POST, POST)                                                            \
  XX(4, PUT, PUT)

#define HTTP_STATUS_MAP(XX)                                                    \
  XX(100, CONTINUE, Continue)                                                  \
  XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                            \
  XX(102, PROCESSING, Processing)                                              \
  XX(200, OK, OK)                                                              \
  XX(201, CREATED, Created)                                                    \
  XX(202, ACCEPTED, Accepted)                                                  \
  XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)      \
  XX(204, NO_CONTENT, No Content)                                              \
  XX(205, RESET_CONTENT, Reset Content)                                        \
  XX(206, PARTIAL_CONTENT, Partial Content)                                    \
  XX(207, MULTI_STATUS, Multi - Status)                                        \
  XX(208, ALREADY_REPORTED, Already Reported)                                  \
  XX(226, IM_USED, IM Used)                                                    \
  XX(300, MULTIPLE_CHOICES, Multiple Choices)                                  \
  XX(301, MOVED_PERMANENTLY, Moved Permanently)                                \
  XX(302, FOUND, Found)                                                        \
  XX(303, SEE_OTHER, See Other)                                                \
  XX(304, NOT_MODIFIED, Not Modified)                                          \
  XX(305, USE_PROXY, Use Proxy)                                                \
  XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                              \
  XX(308, PERMANENT_REDIRECT, Permanent Redirect)                              \
  XX(400, BAD_REQUEST, Bad Request)                                            \
  XX(401, UNAUTHORIZED, Unauthorized)                                          \
  XX(402, PAYMENT_REQUIRED, Payment Required)                                  \
  XX(403, FORBIDDEN, Forbidden)                                                \
  XX(404, NOT_FOUND, Not Found)                                                \
  XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                              \
  XX(406, NOT_ACCEPTABLE, Not Acceptable)                                      \
  XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)        \
  XX(408, REQUEST_TIMEOUT, Request Timeout)                                    \
  XX(409, CONFLICT, Conflict)                                                  \
  XX(410, GONE, Gone)                                                          \
  XX(411, LENGTH_REQUIRED, Length Required)                                    \
  XX(412, PRECONDITION_FAILED, Precondition Failed)                            \
  XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                                \
  XX(414, URI_TOO_LONG, URI Too Long)                                          \
  XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                      \
  XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                        \
  XX(417, EXPECTATION_FAILED, Expectation Failed)                              \
  XX(421, MISDIRECTED_REQUEST, Misdirected Request)                            \
  XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                          \
  XX(423, LOCKED, Locked)                                                      \
  XX(424, FAILED_DEPENDENCY, Failed Dependency)                                \
  XX(426, UPGRADE_REQUIRED, Upgrade Required)                                  \
  XX(428, PRECONDITION_REQUIRED, Precondition Required)                        \
  XX(429, TOO_MANY_REQUESTS, Too Many Requests)                                \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large)    \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)        \
  XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                        \
  XX(501, NOT_IMPLEMENTED, Not Implemented)                                    \
  XX(502, BAD_GATEWAY, Bad Gateway)                                            \
  XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                            \
  XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                    \
  XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)              \
  XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                    \
  XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                          \
  XX(508, LOOP_DETECTED, Loop Detected)                                        \
  XX(510, NOT_EXTENDED, Not Extended)                                          \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

enum class HttpMethod {
#define XX(num, name, string) name = (num),
  HTTP_METHOD_MAP(XX)
#undef XX
      INVALID_METHOD
};

enum class HttpStatus {
#define XX(code, name, desc) name = (code),
  HTTP_STATUS_MAP(XX)
#undef XX
};

HttpMethod stringToHttpMethod(const std::string &m);
HttpMethod charsToHttpMethod(const char *m);

const char *methodToString(const HttpMethod &m);
const char *statusToString(const HttpStatus &s);

struct CaseInsensitiveLess {
  bool operator()(const std::string &lhs, const std::string &rhs) const {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
  }
};

using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

class HttpRequest {
public:
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

  // void init();
  /// TODO: confusing four method
  void initParam();
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
    receive_time_.swap(other.receive_time_);
  }

private:
  HttpMethod method_;
  uint8_t version_;
  bool close_;
  bool websocket_;

  uint8_t parser_param_flag_;

  std::string path_;
  std::string query_;
  std::string fragment_;
  std::string body_;
  MapType headers_;
  MapType params_;
  MapType cookies_;

  Timestamp receive_time_;
};

} // namespace lynx

#endif
