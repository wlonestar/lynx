#include "lynx/http/http_context.h"
#include "lynx/http/http_parser.h"
#include "lynx/http/http_request.h"
#include "lynx/logger/logging.h"
#include "lynx/net/buffer.h"

void testParseRequestAllInOne() {
  lynx::HttpContext context;
  context.start();

  std::string msg("POST /index.html HTTP/1.1\r\n"
                  "Host: www.lynx.com\r\n"
                  "Content-Length: 123\r\n"
                  "Content-Type: application/json\r\n"
                  "\r\n"
                  "{\r\n    \"id\": 123,\r\n    \"name\": \"Li Ming\",\r\n    "
                  "\"gender\": 1,\r\n    \"entry_year\": 2020,\r\n    "
                  "\"major\": \"CS\",\r\n    \"gpa\": 3.5\r\n}");

  LOG_INFO << msg;
  LOG_INFO << context.parseRequest(msg.data(), msg.size());

  LOG_INFO << lynx::methodToString(context.request().method());
  LOG_INFO << context.request().path();
  LOG_INFO << context.request().version();
  LOG_INFO << context.request().body();
}

int main() { testParseRequestAllInOne(); }
