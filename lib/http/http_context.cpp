#include "lynx/http/http_context.h"
#include "lynx/net/buffer.h"

namespace lynx {

bool HttpContext::processRequestLine(const char *begin, const char *end) {
  bool succeed = false;
  const char *start = begin;
  const char *space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space)) {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end) {
      const char *question = std::find(start, space, '?');
      if (question != space) {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      } else {
        request_.setPath(start, space);
      }
      start = space + 1;
      succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (succeed) {
        if (*(end - 1) == '1') {
          request_.setVersion(HttpRequest::HTTP11);
        } else if (*(end - 1) == '0') {
          request_.setVersion(HttpRequest::HTTP10);
        } else {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

bool HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime) {
  bool ok = true;
  bool has_more = true;
  while (has_more) {
    if (state_ == EXPECT_REQUEST_LINE) {
      const char *crlf = buf->findCRLF();
      if (crlf != nullptr) {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok) {
          request_.setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = EXPECT_HEADERS;
        } else {
          has_more = false;
        }
      } else {
        has_more = false;
      }
    } else if (state_ == EXPECT_HEADERS) {
      const char *crlf = buf->findCRLF();
      if (crlf != nullptr) {
        const char *colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf) {
          request_.addHeader(buf->peek(), colon, crlf);
        } else {
          state_ = GOT_ALL;
          has_more = false;
        }
        buf->retrieveUntil(crlf + 2);
      } else {
        has_more = false;
      }
    } else if (state_ == EXPECT_BODY) {
    }
  }
  return ok;
}

} // namespace lynx
