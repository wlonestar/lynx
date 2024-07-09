#include "lynx/logger/logging.h"
#include "lynx/net/inet_address.h"

//#define BOOST_TEST_MODULE InetAddressTest
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(testInetAddress) {
  lynx::InetAddress addr0(1234);
  BOOST_CHECK_EQUAL(addr0.toIp(), std::string("0.0.0.0"));
  BOOST_CHECK_EQUAL(addr0.toIpPort(), std::string("0.0.0.0:1234"));
  BOOST_CHECK_EQUAL(addr0.port(), 1234);

  lynx::InetAddress addr1(4321, true);
  BOOST_CHECK_EQUAL(addr1.toIp(), std::string("127.0.0.1"));
  BOOST_CHECK_EQUAL(addr1.toIpPort(), std::string("127.0.0.1:4321"));
  BOOST_CHECK_EQUAL(addr1.port(), 4321);

  lynx::InetAddress addr2("1.2.3.4", 8888);
  BOOST_CHECK_EQUAL(addr2.toIp(), std::string("1.2.3.4"));
  BOOST_CHECK_EQUAL(addr2.toIpPort(), std::string("1.2.3.4:8888"));
  BOOST_CHECK_EQUAL(addr2.port(), 8888);

  lynx::InetAddress addr3("255.254.253.252", 65535);
  BOOST_CHECK_EQUAL(addr3.toIp(), std::string("255.254.253.252"));
  BOOST_CHECK_EQUAL(addr3.toIpPort(), std::string("255.254.253.252:65535"));
  BOOST_CHECK_EQUAL(addr3.port(), 65535);
}

BOOST_AUTO_TEST_CASE(testInet6Address) {
  lynx::InetAddress addr0(1234, false, true);
  BOOST_CHECK_EQUAL(addr0.toIp(), std::string("::"));
  BOOST_CHECK_EQUAL(addr0.toIpPort(), std::string("[::]:1234"));
  BOOST_CHECK_EQUAL(addr0.port(), 1234);

  lynx::InetAddress addr1(1234, true, true);
  BOOST_CHECK_EQUAL(addr1.toIp(), std::string("::1"));
  BOOST_CHECK_EQUAL(addr1.toIpPort(), std::string("[::1]:1234"));
  BOOST_CHECK_EQUAL(addr1.port(), 1234);

  lynx::InetAddress addr2("2001:db8::1", 8888, true);
  BOOST_CHECK_EQUAL(addr2.toIp(), std::string("2001:db8::1"));
  BOOST_CHECK_EQUAL(addr2.toIpPort(), std::string("[2001:db8::1]:8888"));
  BOOST_CHECK_EQUAL(addr2.port(), 8888);

  lynx::InetAddress addr3("fe80::1234:abcd:1", 8888);
  BOOST_CHECK_EQUAL(addr3.toIp(), std::string("fe80::1234:abcd:1"));
  BOOST_CHECK_EQUAL(addr3.toIpPort(), std::string("[fe80::1234:abcd:1]:8888"));
  BOOST_CHECK_EQUAL(addr3.port(), 8888);
}

BOOST_AUTO_TEST_CASE(testInetAddressResolve) {
  lynx::InetAddress addr(80);
  if (lynx::InetAddress::resolve("google.com", &addr)) {
    LOG_INFO << "google.com resolved to " << addr.toIpPort();
  } else {
    LOG_ERROR << "Unable to resolve google.com";
  }
}
