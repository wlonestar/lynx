#ifndef LYNX_NET_SOCKET_H
#define LYNX_NET_SOCKET_H

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
class Socket {
public:
  /**
   * @brief Constructs a Socket with a given file descriptor.
   *
   * @param sockfd The file descriptor for the socket.
   */
  explicit Socket(int Sockfd) : Sockfd(Sockfd) {}

  /**
   * @brief Destructor for Socket.
   *
   * Closes the socket file descriptor if it is open.
   */
  ~Socket();

  /// Gets the file descriptor of the socket.
  int fd() const { return Sockfd; }

  bool getTcpInfo(struct tcp_info *) const;
  bool getTcpInfoString(char *Buf, int Len) const;

  /**
   * @brief Binds the socket to a local address.
   *
   * @param localaddr The local address to bind the socket to.
   */
  void bindAddress(const InetAddress &Localaddr);

  /**
   * @brief Puts the socket into listening mode.
   *
   * The socket will be ready to accept incoming connections.
   */
  void listen();

  /**
   * @brief Accepts an incoming connection.
   *
   * @param peeraddr Pointer to an InetAddress to store the peer address.
   *
   * @return The file descriptor for the accepted connection.
   */
  int accept(InetAddress *Peeraddr);

  /// Shuts down the writing side of the socket.
  void shutdownWrite();

  void setTcpNoDelay(bool On);
  void setReuseAddr(bool On);
  void setReusePort(bool On);
  void setKeepAlive(bool On);

private:
  const int Sockfd; /// The file descriptor for the socket.
};

} // namespace lynx

#endif
