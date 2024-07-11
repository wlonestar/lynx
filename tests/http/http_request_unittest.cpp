#include "lynx/http/http_context.h"
#include "lynx/net/buffer.h"

//#define BOOST_TEST_MODULE BufferTest
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(testParseRequestAllInOne) {
  lynx::HttpContext context;
  lynx::Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.lynx.com\r\n"
               "\r\n");

  BOOST_CHECK(context.parseRequest(&input, lynx::Timestamp::now()));
  BOOST_CHECK(context.gotAll());
  const lynx::HttpRequest &request = context.request();
  BOOST_CHECK_EQUAL(request.method(), lynx::HttpRequest::GET);
  BOOST_CHECK_EQUAL(request.path(), std::string("/index.html"));
  BOOST_CHECK_EQUAL(request.getVersion(), lynx::HttpRequest::HTTP11);
  BOOST_CHECK_EQUAL(request.getHeader("Host"), std::string("www.lynx.com"));
  BOOST_CHECK_EQUAL(request.getHeader("User-Agent"), std::string(""));
}

BOOST_AUTO_TEST_CASE(testParseRequestInTwoPieces) {
  std::string all("GET /index.html HTTP/1.1\r\n"
                  "Host: www.lynx.com\r\n"
                  "\r\n");

  for (size_t sz1 = 0; sz1 < all.size(); ++sz1) {
    lynx::HttpContext context;
    lynx::Buffer input;
    input.append(all.c_str(), sz1);
    BOOST_CHECK(context.parseRequest(&input, lynx::Timestamp::now()));
    BOOST_CHECK(!context.gotAll());

    size_t sz2 = all.size() - sz1;
    input.append(all.c_str() + sz1, sz2);
    BOOST_CHECK(context.parseRequest(&input, lynx::Timestamp::now()));
    BOOST_CHECK(context.gotAll());
    const lynx::HttpRequest &request = context.request();
    BOOST_CHECK_EQUAL(request.method(), lynx::HttpRequest::GET);
    BOOST_CHECK_EQUAL(request.path(), std::string("/index.html"));
    BOOST_CHECK_EQUAL(request.getVersion(), lynx::HttpRequest::HTTP11);
    BOOST_CHECK_EQUAL(request.getHeader("Host"), std::string("www.lynx.com"));
    BOOST_CHECK_EQUAL(request.getHeader("User-Agent"), std::string(""));
  }
}

BOOST_AUTO_TEST_CASE(testParseRequestEmptyHeaderValue) {
  lynx::HttpContext context;
  lynx::Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.lynx.com\r\n"
               "User-Agent:\r\n"
               "Accept-Encoding: \r\n"
               "\r\n");

  BOOST_CHECK(context.parseRequest(&input, lynx::Timestamp::now()));
  BOOST_CHECK(context.gotAll());
  const lynx::HttpRequest &request = context.request();
  BOOST_CHECK_EQUAL(request.method(), lynx::HttpRequest::GET);
  BOOST_CHECK_EQUAL(request.path(), std::string("/index.html"));
  BOOST_CHECK_EQUAL(request.getVersion(), lynx::HttpRequest::HTTP11);
  BOOST_CHECK_EQUAL(request.getHeader("Host"), std::string("www.lynx.com"));
  BOOST_CHECK_EQUAL(request.getHeader("User-Agent"), std::string(""));
  BOOST_CHECK_EQUAL(request.getHeader("Accept-Encoding"), std::string(""));
}
