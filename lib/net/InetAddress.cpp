#include "lynx/logger/Logging.h"
#include "lynx/net/InetAddress.h"

#include <arpa/inet.h>
#include <cassert>
#include <cstddef>
#include <netinet/in.h>

namespace lynx {

static const in_addr_t KInaddrAny = INADDR_ANY;
static const in_addr_t KInaddrLoopback = INADDR_LOOPBACK;

InetAddress::InetAddress(uint16_t Port, bool LoopbackOnly) {
  static_assert(offsetof(InetAddress, Addr) == 0, "addr_ offset 0");
  memset(&Addr, 0, sizeof(Addr));
  Addr.sin_family = AF_INET;
  in_addr_t Ip = LoopbackOnly ? KInaddrLoopback : KInaddrAny;
  Addr.sin_addr.s_addr = htobe32(Ip);
  Addr.sin_port = htobe16(Port);
}

InetAddress::InetAddress(std::string Ip, uint16_t Port) {
  memset(&Addr, 0, sizeof(Addr));
  Addr.sin_family = AF_INET;
  Addr.sin_port = htobe16(Port);
  if (::inet_pton(AF_INET, Ip.c_str(), &Addr.sin_addr) <= 0)
    LOG_SYSERR << "fromIpPort";
}

std::string InetAddress::toIpPort() const {
  char Buf[64] = "";
  ::inet_ntop(AF_INET, &Addr.sin_addr, Buf, sizeof(Buf));
  size_t End = ::strlen(Buf);
  uint16_t Port = ::ntohs(Addr.sin_port);
  snprintf(Buf + End, sizeof(Buf) - End, ":%u", Port);
  return Buf;
}

std::string InetAddress::toIp() const {
  char Buf[64] = "";
  ::inet_ntop(AF_INET, &Addr.sin_addr, Buf, sizeof(Buf));
  return Buf;
}

uint16_t InetAddress::port() const { return ::ntohs(Addr.sin_port); }

} // namespace lynx
