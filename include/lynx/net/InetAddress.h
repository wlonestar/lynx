#ifndef LYNX_NET_INET_ADDRESS_H
#define LYNX_NET_INET_ADDRESS_H

#include <netinet/in.h>
#include <string>

namespace lynx {

/**
 * @class InetAddress
 * @brief A class representing an IPv4 socket address.
 *
 * The InetAddress class provides utilities for handling and manipulating IPv4
 * socket addresses, including coversion between IP address strings and port
 * numbers.
 */
class InetAddress {
public:
  explicit InetAddress(uint16_t Port = 0, bool LoopbackOnly = false);
  InetAddress(std::string Ip, uint16_t Port);
  explicit InetAddress(const struct sockaddr_in &Addr) : Addr(Addr) {}

  /**
   * @brief Converts the address to a string representation of the IP address.
   *
   * @return A string representation of the IP address.
   */
  std::string toIp() const;

  /**
   * @brief Converts the address to a string representation of the IP address
   * and port.
   *
   * @return A string representation of the IP address and port.
   */
  std::string toIpPort() const;

  /// Gets the port number.
  uint16_t port() const;

  const struct sockaddr *getSockAddr() const {
    return static_cast<const struct sockaddr *>(
        static_cast<const void *>(&Addr));
  }
  void setSockAddr(const struct sockaddr_in &Address) { Addr = Address; }

private:
  struct sockaddr_in Addr; /// The underlying sockaddr_in structure.
};

} // namespace lynx

#endif
