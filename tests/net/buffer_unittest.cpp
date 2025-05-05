#include "lynx/net/Buffer.h"

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(testBufferAppendRetrieve) {
  lynx::Buffer buf;
  BOOST_CHECK_EQUAL(buf.readableBytes(), 0);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);

  const std::string str(200, 'x');
  buf.append(str);
  BOOST_CHECK_EQUAL(buf.readableBytes(), str.size());
  BOOST_CHECK_EQUAL(buf.writableBytes(),
                    lynx::Buffer::KInitialSize - str.size());
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);

  const std::string str2 = buf.retrieveAsString(50);
  BOOST_CHECK_EQUAL(str2.size(), 50);
  BOOST_CHECK_EQUAL(buf.readableBytes(), str.size() - str2.size());
  BOOST_CHECK_EQUAL(buf.writableBytes(),
                    lynx::Buffer::KInitialSize - str.size());
  BOOST_CHECK_EQUAL(buf.prependableBytes(),
                    lynx::Buffer::KCheapPrepend + str2.size());
  BOOST_CHECK_EQUAL(str2, std::string(50, 'x'));

  buf.append(str);
  BOOST_CHECK_EQUAL(buf.readableBytes(), 2 * str.size() - str2.size());
  BOOST_CHECK_EQUAL(buf.writableBytes(),
                    lynx::Buffer::KInitialSize - 2 * str.size());
  BOOST_CHECK_EQUAL(buf.prependableBytes(),
                    lynx::Buffer::KCheapPrepend + str2.size());

  const std::string str3 = buf.retrieveAllAsString();
  BOOST_CHECK_EQUAL(str3.size(), 350);
  BOOST_CHECK_EQUAL(buf.readableBytes(), 0);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);
  BOOST_CHECK_EQUAL(str3, std::string(350, 'x'));
}

BOOST_AUTO_TEST_CASE(testBufferGrow) {
  lynx::Buffer buf;
  buf.append(std::string(400, 'y'));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 400);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 400);

  buf.retrieve(50);
  BOOST_CHECK_EQUAL(buf.readableBytes(), 350);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 400);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend + 50);

  buf.append(std::string(1000, 'z'));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 1350);
  BOOST_CHECK_EQUAL(buf.writableBytes(), 0);
  BOOST_CHECK_EQUAL(buf.prependableBytes(),
                    lynx::Buffer::KCheapPrepend + 50); // FIXME

  buf.retrieveAll();
  BOOST_CHECK_EQUAL(buf.readableBytes(), 0);
  BOOST_CHECK_EQUAL(buf.writableBytes(), 1400); // FIXME
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);
}

BOOST_AUTO_TEST_CASE(testBufferInsideGrow) {
  lynx::Buffer buf;
  buf.append(std::string(800, 'y'));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 800);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 800);

  buf.retrieve(500);
  BOOST_CHECK_EQUAL(buf.readableBytes(), 300);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 800);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend + 500);

  buf.append(std::string(300, 'z'));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 600);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 600);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);
}

BOOST_AUTO_TEST_CASE(testBufferShrink) {
  lynx::Buffer buf;
  buf.append(std::string(2000, 'y'));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 2000);
  BOOST_CHECK_EQUAL(buf.writableBytes(), 0);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);

  buf.retrieve(1500);
  BOOST_CHECK_EQUAL(buf.readableBytes(), 500);
  BOOST_CHECK_EQUAL(buf.writableBytes(), 0);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend + 1500);

  buf.shrink(0);
  BOOST_CHECK_EQUAL(buf.readableBytes(), 500);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 500);
  BOOST_CHECK_EQUAL(buf.retrieveAllAsString(), std::string(500, 'y'));
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);
}

BOOST_AUTO_TEST_CASE(testBufferPrepend) {
  lynx::Buffer buf;
  buf.append(std::string(200, 'y'));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 200);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 200);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend);

  int x = 0;
  buf.prepend(&x, sizeof(x));
  BOOST_CHECK_EQUAL(buf.readableBytes(), 204);
  BOOST_CHECK_EQUAL(buf.writableBytes(), lynx::Buffer::KInitialSize - 200);
  BOOST_CHECK_EQUAL(buf.prependableBytes(), lynx::Buffer::KCheapPrepend - 4);
}

BOOST_AUTO_TEST_CASE(testBufferFindEOL) {
  lynx::Buffer buf;
  buf.append(std::string(100000, 'x'));
  const char *null = nullptr;
  BOOST_CHECK_EQUAL(buf.findEOL(), null);
  BOOST_CHECK_EQUAL(buf.findEOL(buf.peek() + 90000), null);
}

void output(lynx::Buffer &&buf, const void *inner) {
  lynx::Buffer newbuf(std::move(buf));
  // printf("New Buffer at %p, inner %p\n", &newbuf, newbuf.peek());
  BOOST_CHECK_EQUAL(inner, newbuf.peek());
}

// NOTE: This test fails in g++ 4.4, passes in g++ 4.6.
BOOST_AUTO_TEST_CASE(testMove) {
  lynx::Buffer buf;
  buf.append("lynx", 5);
  const void *inner = buf.peek();
  // printf("Buffer at %p, inner %p\n", &buf, inner);
  output(std::move(buf), inner);
}
