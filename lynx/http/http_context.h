#ifndef LYNX_HTTP_HTTP_CONTEXT_H
#define LYNX_HTTP_HTTP_CONTEXT_H

#include "lynx/http/http_parser.h"
#include "lynx/http/http_request.h"

namespace lynx {

class HttpContext {
public:
  HttpContext();

  void start();

  bool parseRequest(char *data, size_t len);

  bool isFinished();
  bool hasError();

  HttpRequest &request() { return request_; }
  HttpParser &parser() { return parser_; }

  uint64_t getContentLength();

private:
  HttpRequest request_;
  HttpParser parser_;
  int error_;
};

} // namespace lynx

#endif
