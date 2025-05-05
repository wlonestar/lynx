#ifndef LYNX_HTTP_HTTP_PARSER_H
#define LYNX_HTTP_HTTP_PARSER_H

#include "lynx/http/HttpRequest.h"

namespace lynx {

using ElementCallback = void (*)(void *, const char *, size_t);
using FieldCallback = void (*)(void *, const char *, size_t, const char *,
                               size_t);

/**
 * @struct HttpParser
 * @brief Struct representing an HTTP parser.
 *
 * This struct represents an HTTP parser and provides methods for parsing HTTP
 * messages. It uses the Ragel state machine to parse HTTP headers and it
 * provides callbacks for parsing different parts of the HTTP message.
 */
struct HttpParser {
  int CS;            /// Current state of the parser
  size_t BodyStart;  /// Start position of the HTTP message body
  int ContentLen;    /// Length of the content
  size_t Nread;      /// Total number of bytes read so far
  size_t Mark;       /// Mark position for some operations
  size_t FieldStart; /// Start position of the field
  size_t FieldLen;   /// Length of the field
  size_t QueryStart; /// Start position of the query string
  int XmlSent;       /// Flag indicating whether XML has been sent
  int JsonSent;      /// Flag indicating whether JSON has been sent
  void *Data;        /// Pointer to additional data

  int UriRelaxed; /// Flag indicating whether the URI is relaxed

  /// Callback for parsing HTTP fields.
  FieldCallback HttpField;

  /// Callback for parsing the request method.
  ElementCallback RequestMethod;

  /// Callback for parsing the request URI.
  ElementCallback RequestUri;

  /// Callback for parsing the fragment identifier.
  ElementCallback Fragment;

  /// Callback for parsing the request path.
  ElementCallback RequestPath;

  /// Callback for parsing the query string.
  ElementCallback QueryString;

  /// Callback for parsing the HTTP version.
  ElementCallback HttpVersion;

  /// Callback indicating header parse completion.
  ElementCallback HeaderDone;

  HttpParser() { init(); }

  ~HttpParser() = default;

  /// Initializes the parser.
  int init();

  /// Finishes the parsing process.
  int finish();

  /**
   * @brief Executes the parser on a buffer.
   *
   * Parses the buffer and updates the parser's state accordingly.
   *
   * @param buffer Pointer to the buffer.
   * @param len Length of the buffer.
   * @param off Offset in the buffer.
   * @return The number of bytes parsed.
   */
  size_t execute(const char *Buffer, size_t Len, size_t Off);

  /// Checks if the parser has an error.
  int hasError();

  /// Checks if the parsing is finished.
  bool isFinished();
};

} // namespace lynx

#endif
