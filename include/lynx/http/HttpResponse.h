#ifndef LYNX_HTTP_HTTP_RESPONSE_H
#define LYNX_HTTP_HTTP_RESPONSE_H

#include "lynx/http/HttpStatus.h"

#include <map>
#include <string>

namespace lynx {

class Buffer;

/**
 * @class HttpResponse
 * @brief Represents an HTTP response.
 *
 * This class encapsulates the information in an HTTP response, including the
 * response status code, headers, and body. It provides methods for setting and
 * getting these attributes, as well as methods for adding headers.
 */
class HttpResponse {
public:
  /**
   * @brief Constructs a new HttpResponse.
   *
   * @param close A boolean indicating whether the connection should be closed
   * after the response is sent.
   */
  explicit HttpResponse(bool Close) : CloseConnection(Close) {}

  void setStatusCode(HttpStatus Stats) { Status = Stats; }
  void setCloseConnection(bool On) { CloseConnection = On; }

  void setBody(const std::string &Bod) { Body = Bod; }
  void setContentType(const std::string &ContentType) {
    addHeader("Content-Type", ContentType);
  }

  bool closeConnection() const { return CloseConnection; }

  void addHeader(const std::string &Key, const std::string &Value) {
    Headers[Key] = Value;
  }

  void appendToBuffer(Buffer *Output) const;

private:
  std::map<std::string, std::string> Headers;
  HttpStatus Status;
  bool CloseConnection;
  std::string Body;
};

} // namespace lynx

#endif
