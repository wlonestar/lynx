#include "lynx/http/HttpContext.h"
#include "lynx/logger/Logging.h"

namespace lynx {

namespace detail {

void onRequestHttpField(void *Data, const char *Field, size_t Flen,
                        const char *Value, size_t Vlen) {
  auto *Context = static_cast<HttpContext *>(Data);
  if (Flen == 0) {
    LOG_WARN << "invalid http request Field Length == 0";
    return;
  }
  Context->request().setHeader(std::string(Field, Flen),
                               std::string(Value, Vlen));
}

void onRequestMethod(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  HttpMethod M = charsToHttpMethod(At);
  if (M == HttpMethod::INVALID_METHOD) {
    LOG_WARN << "Invalid http request method: " << std::string(At, Length);
    return;
  }
  Context->request().setMethod(M);
}

void onRequestUri(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  Context->request().setUri(std::string(At, Length));
}

void onRequestFragment(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  Context->request().setFragment(std::string(At, Length));
}

void onRequestPath(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  Context->request().setPath(std::string(At, Length));
}

void onRequestQuery(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  Context->request().setQuery(std::string(At, Length));
  Context->request().initQueryParam(); /// Init query params
}

void onRequestVersion(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  uint8_t V = 0;
  if (strncmp(At, "HTTP/1.1", Length) == 0) {
    V = 0x11;
  } else if (strncmp(At, "HTTP/1.0", Length) == 0) {
    V = 0x10;
  } else {
    LOG_WARN << "Invalid http request version: " << std::string(At, Length);
    return;
  }
  Context->request().setVersion(V);
}

void onRequestHeaderDone(void *Data, const char *At, size_t Length) {
  auto *Context = static_cast<HttpContext *>(Data);
  Context->request().initCookies(); /// Init cookies
  /// Set parser's content Length
  std::string ContentLen = Context->request().getHeader("content-Length");
  Context->parser().ContentLen = atoi(ContentLen.c_str());
}

} // namespace detail

HttpContext::HttpContext() : Request(), Parser(), Error(0) {}

void HttpContext::start() {
  Parser.HttpField = detail::onRequestHttpField;
  Parser.RequestMethod = detail::onRequestMethod;
  Parser.RequestUri = detail::onRequestUri;
  Parser.Fragment = detail::onRequestFragment;
  Parser.RequestPath = detail::onRequestPath;
  Parser.QueryString = detail::onRequestQuery;
  Parser.HttpVersion = detail::onRequestVersion;
  Parser.HeaderDone = detail::onRequestHeaderDone;
  Parser.Data = this;
}

bool HttpContext::parseRequest(char *Data, size_t Len) {
  size_t Offset = Parser.execute(Data, Len, 0);
  (void)Offset;

  /// Set body
  Request.setBody(std::string(Data + Parser.BodyStart, Parser.ContentLen));
  Request.initBodyParam();

  return Parser.isFinished() && !Parser.hasError();
}

bool HttpContext::isFinished() { return Parser.isFinished(); }
bool HttpContext::hasError() { return Error || Parser.hasError(); }

} // namespace lynx
