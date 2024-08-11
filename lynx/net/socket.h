#ifndef LYNX_NET_SOCKET_H
#define LYNX_NET_SOCKET_H

#include "lynx/base/noncopyable.h"

#include <netinet/tcp.h>

namespace lynx {

class InetAddress;

/**
 * @class Socket
 * @brief A wrapper around a socket file descriptor, providing common socket
 * operations.
 *
 * The Socket class provides a RAII-style wrapper around a socket file
 * descriptor, offering various methods to manipulate socket options and perform
 * network operations.
 */
class Socket : Noncopyable {
public:
  /**
   * @brief Constructs a Socket with a given file descriptor.
   * @param sockfd The file descriptor for the socket.
   */
  explicit Socket(int sockfd) : sockfd_(sockfd) {}

  /**
   * @brief Destructor for Socket.
   *
   * Closes the socket file descriptor if it is open.
   */
  ~Socket();

  /// Gets the file descriptor of the socket.
  int fd() const { return sockfd_; }

  bool getTcpInfo(struct tcp_info *) const;
  bool getTcpInfoString(char *buf, int len) const;

  /**
   * @brief Binds the socket to a local address.
   * @param localaddr The local address to bind the socket to.
   */
  void bindAddress(const InetAddress &localaddr);

  /**
   * @brief Puts the socket into listening mode.
   *
   * The socket will be ready to accept incoming connections.
   */
  void listen();

  /**
   * @brief Accepts an incoming connection.
   * @param peeraddr Pointer to an InetAddress to store the peer address.
   * @return The file descriptor for the accepted connection.
   */
  int accept(InetAddress *peeraddr);

  /// Shuts down the writing side of the socket.
  void shutdownWrite();

  void setTcpNoDelay(bool on);
  void setReuseAddr(bool on);
  void setReusePort(bool on);
  void setKeepAlive(bool on);

private:
  const int sockfd_; /// The file descriptor for the socket.
};

} // namespace lynx

#endif
