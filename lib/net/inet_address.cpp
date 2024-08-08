#include "lynx/net/inet_address.h"
#include "lynx/logger/logging.h"

#include <arpa/inet.h>
#include <cassert>
#include <cstddef>
#include <netinet/in.h>

namespace lynx {

static const in_addr_t K_INADDR_ANY = INADDR_ANY;
static const in_addr_t K_INADDR_LOOPBACK = INADDR_LOOPBACK;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly) {
  static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  in_addr_t ip = loopbackOnly ? K_INADDR_LOOPBACK : K_INADDR_ANY;
  addr_.sin_addr.s_addr = htobe32(ip);
  addr_.sin_port = htobe16(port);
}

InetAddress::InetAddress(std::string ip, uint16_t port) {
  memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_port = htobe16(port);
  if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
    LOG_SYSERR << "fromIpPort";
  }
}

std::string InetAddress::toIpPort() const {
  char buf[64] = "";
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
  size_t end = ::strlen(buf);
  uint16_t port = ::ntohs(addr_.sin_port);
  snprintf(buf + end, sizeof(buf) - end, ":%u", port);
  return buf;
}

std::string InetAddress::toIp() const {
  char buf[64] = "";
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
  return buf;
}

uint16_t InetAddress::port() const { return ::ntohs(addr_.sin_port); }

} // namespace lynx
