#ifndef LYNX_HTTP_HTTP_CONTEXT_H
#define LYNX_HTTP_HTTP_CONTEXT_H

#include "lynx/base/timestamp.h"
#include "lynx/http/http_request.h"

namespace lynx {

class Buffer;

class HttpContext {
public:
  enum HttpRequestParseState {
    EXPECT_REQUEST_LINE,
    EXPECT_HEADERS,
    EXPECT_BODY,
    GOT_ALL,
  };

  HttpContext() = default;

  bool parseRequest(Buffer *buf, Timestamp receiveTime);

  bool gotAll() const { return state_ == GOT_ALL; }

  void reset() {
    state_ = EXPECT_REQUEST_LINE;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest &request() const { return request_; }
  HttpRequest &request() { return request_; }

private:
  bool processRequestLine(const char *begin, const char *end);

  HttpRequestParseState state_{};
  HttpRequest request_;
};

} // namespace lynx

#endif
