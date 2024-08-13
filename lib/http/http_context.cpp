#include "lynx/http/http_context.h"
#include "lynx/logger/logging.h"

namespace lynx {

namespace detail {

void onRequestHttpField(void *data, const char *field, size_t flen,
                        const char *value, size_t vlen) {
  auto *context = static_cast<HttpContext *>(data);
  if (flen == 0) {
    LOG_WARN << "invalid http request field length == 0";
    return;
  }
  context->request().setHeader(std::string(field, flen),
                               std::string(value, vlen));
}

void onRequestMethod(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  HttpMethod m = charsToHttpMethod(at);
  if (m == HttpMethod::INVALID_METHOD) {
    LOG_WARN << "Invalid http request method: " << std::string(at, length);
    return;
  }
  context->request().setMethod(m);
}

void onRequestUri(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  context->request().setUri(std::string(at, length));
}

void onRequestFragment(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  context->request().setFragment(std::string(at, length));
}

void onRequestPath(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  context->request().setPath(std::string(at, length));
}

void onRequestQuery(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  context->request().setQuery(std::string(at, length));
  /// Init query params
  context->request().initQueryParam();
}

void onRequestVersion(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  uint8_t v = 0;
  if (strncmp(at, "HTTP/1.1", length) == 0) {
    v = 0x11;
  } else if (strncmp(at, "HTTP/1.0", length) == 0) {
    v = 0x10;
  } else {
    LOG_WARN << "Invalid http request version: " << std::string(at, length);
    return;
  }
  context->request().setVersion(v);
}

void onRequestHeaderDone(void *data, const char *at, size_t length) {
  auto *context = static_cast<HttpContext *>(data);
  /// Init cookies
  context->request().initCookies();
  /// Set parser's content length
  std::string content_len = context->request().getHeader("content-length");
  context->parser().content_len_ = atoi(content_len.c_str());
}

} // namespace detail

HttpContext::HttpContext() : request_(), parser_(), error_(0) {}

void HttpContext::start() {
  parser_.http_field_ = detail::onRequestHttpField;
  parser_.request_method_ = detail::onRequestMethod;
  parser_.request_uri_ = detail::onRequestUri;
  parser_.fragment_ = detail::onRequestFragment;
  parser_.request_path_ = detail::onRequestPath;
  parser_.query_string_ = detail::onRequestQuery;
  parser_.http_version_ = detail::onRequestVersion;
  parser_.header_done_ = detail::onRequestHeaderDone;
  parser_.data_ = this;
}

bool HttpContext::parseRequest(char *data, size_t len) {
  size_t offset = parser_.execute(data, len, 0);
  (void)offset;

  /// Set body
  request_.setBody(
      std::string(data + parser_.body_start_, parser_.content_len_));
  request_.initBodyParam();

  return parser_.isFinished() && !parser_.hasError();
}

bool HttpContext::isFinished() { return parser_.isFinished(); }
bool HttpContext::hasError() { return error_ || parser_.hasError(); }

} // namespace lynx
