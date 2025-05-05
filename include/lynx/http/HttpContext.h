#ifndef LYNX_HTTP_HTTP_CONTEXT_H
#define LYNX_HTTP_HTTP_CONTEXT_H

#include "lynx/http/HttpParser.h"
#include "lynx/http/HttpRequest.h"

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
  bool parseRequest(char *Data, size_t Len);

  /// Checks if the parsing of the HTTP request is finished.
  bool isFinished();

  /// Checks if there is an error in parsing the HTTP request.
  bool hasError();

  /// Returns a reference to the HttpRequest object.
  HttpRequest &request() { return Request; }

  /// Returns a reference to the HttpParser object.
  HttpParser &parser() { return Parser; }

private:
  HttpRequest Request;
  HttpParser Parser;
  int Error;
};

} // namespace lynx

#endif
