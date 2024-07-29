#ifndef LYNX_HTTP_HTTP_PARSER_H
#define LYNX_HTTP_HTTP_PARSER_H

#include "lynx/http/http_request.h"

namespace lynx {

using ElementCallback = void (*)(void *, const char *, size_t);
using FieldCallback = void (*)(void *, const char *, size_t, const char *,
                               size_t);

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
  FieldCallback http_field_;
  ElementCallback request_method_;
  ElementCallback request_uri_;
  ElementCallback fragment_;
  ElementCallback request_path_;
  ElementCallback query_string_;
  ElementCallback http_version_;
  ElementCallback header_done_;

  HttpParser() { init(); }
  ~HttpParser() = default;

  int init();
  int finish();
  size_t execute(const char *buffer, size_t len, size_t off);
  int hasError();
  bool isFinished();
};

} // namespace lynx

#endif
