#ifndef LYNX_HTTP_HTTP_REQUEST_H
#define LYNX_HTTP_HTTP_REQUEST_H

#include "lynx/base/Timestamp.h"
#include "lynx/http/HttpStatus.h"

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

HttpMethod stringToHttpMethod(const std::string &M);
HttpMethod charsToHttpMethod(const char *M);
const char *methodToString(const HttpMethod &M);

struct CaseInsensitiveLess {
  bool operator()(const std::string &Lhs, const std::string &Rhs) const {
    return strcasecmp(Lhs.c_str(), Rhs.c_str()) < 0;
  }
};

/**
 * @class HttpRequest
 * @brief A class that Represents an HTTP request
 *
 * This class encapsulates the information in an HTTP request, including the
 * request method, version, connection status, WebSocket status, header fields,
 * parameter fields, cookie fields, and a buffer for storing the request body.
 * It provides methods for setting and getting these attributes, as well as
 * methods for setting, getting, and deleting headers, parameters, and cookies.
 */
class HttpRequest {
public:
  /// Uses a std::map with a custom comparison method to store header
  using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

  HttpRequest(uint8_t Version = 0x11, bool Close = true);

  bool isClose() const { return Close; }
  void setClose(bool C) { Close = C; }

  bool isWebsocket() const { return Websocket; }
  void setWebsocket(bool W) { Websocket = W; }

  HttpMethod method() const { return Method; }
  void setMethod(HttpMethod M) { Method = M; }

  uint8_t version() const { return Version; }
  void setVersion(uint8_t V) { Version = V; }

  const std::string &path() const { return Path; }
  void setPath(const std::string &P) { Path = P; }

  const std::string &query() const { return Query; }
  void setQuery(const std::string &Q) { Query = Q; }

  const std::string &uri() const { return Uri; }
  void setUri(const std::string &U) { Uri = U; }

  void setFragment(const std::string &F) { Fragment = F; }

  const std::string &body() const { return Body; }
  void setBody(const std::string &B) { Body = B; }

  const MapType &headers() const { return Headers; }
  void setHeaders(const MapType &H) { Headers = H; }

  const MapType &params() const { return Params; }
  void setParams(const MapType &P) { Params = P; }

  const MapType &cookies() const { return Cookies; }
  void setCookies(const MapType &C) { Cookies = C; }

  std::string getHeader(const std::string &Key,
                        const std::string &Def = "") const;
  std::string getParam(const std::string &Key,
                       const std::string &Def = "") const;
  std::string getCookie(const std::string &Key, const std::string &Def = "");

  void setHeader(const std::string &Key, const std::string &Val);
  void setParam(const std::string &Key, const std::string &Val);
  void setCookie(const std::string &Key, const std::string &Val);

  void delHeader(const std::string &Key);
  void delParam(const std::string &Key);
  void delCookie(const std::string &Key);

  bool hasHeader(const std::string &Key, std::string *Val = nullptr);
  bool hasParam(const std::string &Key, std::string *Val = nullptr);
  bool hasCookie(const std::string &Key, std::string *Val = nullptr);

  std::ostream &dump(std::ostream &OS) const;

  std::string toString() const;

  void initQueryParam();
  void initBodyParam();
  void initCookies();

  void swap(HttpRequest &Other) {
    std::swap(Method, Other.Method);
    std::swap(Version, Other.Version);
    std::swap(Close, Other.Close);
    std::swap(Websocket, Other.Websocket);
    std::swap(ParserParamFlag, Other.ParserParamFlag);
    Path.swap(Other.Path);
    Query.swap(Other.Query);
    Fragment.swap(Other.Fragment);
    Body.swap(Other.Body);
    Headers.swap(Other.Headers);
    Params.swap(Other.Params);
    Cookies.swap(Other.Cookies);
  }

private:
  HttpMethod Method;
  uint8_t Version;
  bool Close;
  bool Websocket;

  uint8_t ParserParamFlag;

  std::string Path;
  std::string Query;
  std::string Uri;
  std::string Fragment;
  std::string Body;
  MapType Headers;
  MapType Params;
  MapType Cookies;
};

} // namespace lynx

#endif
