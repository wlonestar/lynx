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
  explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);
  InetAddress(std::string ip, uint16_t port);
  explicit InetAddress(const struct sockaddr_in &addr) : addr_(addr) {}

  /**
   * @brief Converts the address to a string representation of the IP address.
   * @return A string representation of the IP address.
   */
  std::string toIp() const;

  /**
   * @brief Converts the address to a string representation of the IP address
   * and port.
   * @return A string representation of the IP address and port.
   */
  std::string toIpPort() const;

  /// Gets the port number.
  uint16_t port() const;

  const struct sockaddr *getSockAddr() const {
    return static_cast<const struct sockaddr *>(
        static_cast<const void *>(&addr_));
  }
  void setSockAddr(const struct sockaddr_in &addr) { addr_ = addr; }

private:
  struct sockaddr_in addr_; /// The underlying sockaddr_in structure.
};

} // namespace lynx

#endif
