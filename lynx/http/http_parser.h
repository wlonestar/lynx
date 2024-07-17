#ifndef LYNX_HTTP_HTTP_PARSER_H
#define LYNX_HTTP_HTTP_PARSER_H

#include "lynx/http/http_request.h"

namespace lynx {

using element_cb = void (*)(void *, const char *, size_t);
using field_cb = void (*)(void *, const char *, size_t, const char *, size_t);

struct HttpParser {
  int cs_;
  size_t body_start_;
  int content_len_;
  size_t nread_;
  size_t mark_;
  size_t field_start_;
  size_t field_len_;
  size_t query_start_;
  int xml_sent_;
  int json_sent_;

  void *data_;

  int uri_relaxed_;
  field_cb http_field_;
  element_cb request_method_;
  element_cb request_uri_;
  element_cb fragment_;
  element_cb request_path_;
  element_cb query_string_;
  element_cb http_version_;
  element_cb header_done_;

  HttpParser() { init(); }
  ~HttpParser() = default;

  int init();
  int finish();
  size_t execute(const char *buffer, size_t len, size_t off);
  int hasError();
  bool isFinished();
};

#define http_parser_nread(parser) (parser)->nread_

} // namespace lynx

#endif
