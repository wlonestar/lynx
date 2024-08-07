#include "lynx/logger/logging.h"
#include "lynx/net/inet_address.h"

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
