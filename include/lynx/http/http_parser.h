#ifndef LYNX_HTTP_HTTP_PARSER_H
#define LYNX_HTTP_HTTP_PARSER_H

#include "lynx/http/http_request.h"

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
  int cs_;             /// Current state of the parser
  size_t body_start_;  /// Start position of the HTTP message body
  int content_len_;    /// Length of the content
  size_t nread_;       /// Total number of bytes read so far
  size_t mark_;        /// Mark position for some operations
  size_t field_start_; /// Start position of the field
  size_t field_len_;   /// Length of the field
  size_t query_start_; /// Start position of the query string
  int xml_sent_;       /// Flag indicating whether XML has been sent
  int json_sent_;      /// Flag indicating whether JSON has been sent
  void *data_;         /// Pointer to additional data

  int uri_relaxed_; /// Flag indicating whether the URI is relaxed

  /// Callback for parsing HTTP fields.
  FieldCallback http_field_;

  /// Callback for parsing the request method.
  ElementCallback request_method_;

  /// Callback for parsing the request URI.
  ElementCallback request_uri_;

  /// Callback for parsing the fragment identifier.
  ElementCallback fragment_;

  /// Callback for parsing the request path.
  ElementCallback request_path_;

  /// Callback for parsing the query string.
  ElementCallback query_string_;

  /// Callback for parsing the HTTP version.
  ElementCallback http_version_;

  /// Callback indicating header parse completion.
  ElementCallback header_done_;

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
  size_t execute(const char *buffer, size_t len, size_t off);

  /// Checks if the parser has an error.
  int hasError();

  /// Checks if the parsing is finished.
  bool isFinished();
};

} // namespace lynx

#endif
