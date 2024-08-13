#ifndef LYNX_HTTP_HTTP_CONTEXT_H
#define LYNX_HTTP_HTTP_CONTEXT_H

#include "lynx/http/http_parser.h"
#include "lynx/http/http_request.h"

namespace lynx {

/**
 * @class HttpContext
 * @brief Represents the HTTP context, which includes the HTTP request and the
 * parser.
 */
class HttpContext {
public:
  HttpContext();

  /// Starts parsing the HTTP request.
  void start();

  /**
   * @brief Parses the HTTP request.
   *
   * @param data Pointer to the request data.
   * @param len Length of the request data.
   * @return True if the request is parsed successfully, false otherwise.
   */
  bool parseRequest(char *data, size_t len);

  /// Checks if the parsing of the HTTP request is finished.
  bool isFinished();

  /// Checks if there is an error in parsing the HTTP request.
  bool hasError();

  /// Returns a reference to the HttpRequest object.
  HttpRequest &request() { return request_; }

  /// Returns a reference to the HttpParser object.
  HttpParser &parser() { return parser_; }

private:
  HttpRequest request_;
  HttpParser parser_;
  int error_;
};

} // namespace lynx

#endif
