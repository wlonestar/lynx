#include "lynx/net/sockets_ops.h"
#include "lynx/logger/logging.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <endian.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

namespace lynx::sockets {

using SA = struct sockaddr;

const struct sockaddr *sockaddrCast(const struct sockaddr_in6 *addr) {
  return static_cast<const struct sockaddr *>(static_cast<const void *>(addr));
}

struct sockaddr *sockaddrCast(struct sockaddr_in6 *addr) {
  return static_cast<struct sockaddr *>(static_cast<void *>(addr));
}

const struct sockaddr *sockaddrCast(const struct sockaddr_in *addr) {
  return static_cast<const struct sockaddr *>(static_cast<const void *>(addr));
}

const struct sockaddr_in *sockaddrInCast(const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in *>(
      static_cast<const void *>(addr));
}

const struct sockaddr_in6 *sockaddrIn6Cast(const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in6 *>(
      static_cast<const void *>(addr));
}

int createNonblockingOrDie(sa_family_t family) {
#if VALGRIND
  int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_SYSFATAL << "createNonblockingOrDie";
  }

  setNonBlockAndCloseOnExec(sockfd);
#else
  int sockfd =
      ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG_SYSFATAL << "createNonblockingOrDie";
  }
#endif
  return sockfd;
}

void bindOrDie(int sockfd, const struct sockaddr *addr) {
  int ret =
      ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG_SYSFATAL << "bindOrDie";
  }
}

void listenOrDie(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG_SYSFATAL << "listenOrDie";
  }
}

int accept(int sockfd, struct sockaddr_in6 *addr) {
  auto addrlen = static_cast<socklen_t>(sizeof(*addr));
#if VALGRIND || defined(NO_ACCEPT4)
  int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
  setNonBlockAndCloseOnExec(connfd);
#else
  int connfd = ::accept4(sockfd, sockaddrCast(addr), &addrlen,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (connfd < 0) {
    int saved_errno = errno;
    LOG_SYSERR << "Socket::accept";
    switch (saved_errno) {
    case EAGAIN:
    case ECONNABORTED:
    case EINTR:
    case EPROTO:
    case EPERM:
    case EMFILE:
      errno = saved_errno;
      break;
    case EBADF:
    case EFAULT:
    case EINVAL:
    case ENFILE:
    case ENOBUFS:
    case ENOMEM:
    case ENOTSOCK:
    case EOPNOTSUPP:
      LOG_FATAL << "unexpected error of ::accept " << saved_errno;
      break;
    default:
      LOG_FATAL << "unknown error of ::accept " << saved_errno;
      break;
    }
  }
  return connfd;
}

int connect(int sockfd, const struct sockaddr *addr) {
  return ::connect(sockfd, addr,
                   static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

void close(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG_SYSERR << "close";
  }
}

void shutdownWrite(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    LOG_SYSERR << "shutdownWrite";
  }
}

void toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET6) {
    buf[0] = '[';
    toIp(buf + 1, size - 1, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in6 *addr6 = sockaddrIn6Cast(addr);
    uint16_t port = be16toh(addr6->sin6_port);
    assert(size > end);
    snprintf(buf + end, size - end, "]:%u", port);
    return;
  }
  toIp(buf, size, addr);
  size_t end = ::strlen(buf);
  const struct sockaddr_in *addr4 = sockaddrInCast(addr);
  uint16_t port = be16toh(addr4->sin_port);
  assert(size > end);
  snprintf(buf + end, size - end, ":%u", port);
}

void toIp(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET) {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in *addr4 = sockaddrInCast(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  } else if (addr->sa_family == AF_INET6) {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6 *addr6 = sockaddrIn6Cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = htobe16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    LOG_SYSERR << "fromIpPort";
  }
}

void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr) {
  addr->sin6_family = AF_INET6;
  addr->sin6_port = htobe16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
    LOG_SYSERR << "fromIpPort";
  }
}

int getSocketError(int sockfd) {
  int optval;
  auto optlen = static_cast<socklen_t>(sizeof(optval));

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  }
  return optval;
}

struct sockaddr_in6 getLocalAddr(int sockfd) {
  struct sockaddr_in6 localaddr;
  memset(&localaddr, 0, sizeof(localaddr));
  auto addrlen = static_cast<socklen_t>(sizeof(localaddr));
  if (::getsockname(sockfd, sockaddrCast(&localaddr), &addrlen) < 0) {
    LOG_SYSERR << "getLocalAddr";
  }
  return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd) {
  struct sockaddr_in6 peeraddr;
  memset(&peeraddr, 0, sizeof(peeraddr));
  auto addrlen = static_cast<socklen_t>(sizeof(peeraddr));
  if (::getpeername(sockfd, sockaddrCast(&peeraddr), &addrlen) < 0) {
    LOG_SYSERR << "getPeerAddr";
  }
  return peeraddr;
}

bool isSelfConnect(int sockfd) {
  struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
  struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
  if (localaddr.sin6_family == AF_INET) {
    const struct sockaddr_in *laddr4 =
        reinterpret_cast<struct sockaddr_in *>(&localaddr);
    const struct sockaddr_in *raddr4 =
        reinterpret_cast<struct sockaddr_in *>(&peeraddr);
    return laddr4->sin_port == raddr4->sin_port &&
           laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
  }
  if (localaddr.sin6_family == AF_INET6) {
    return localaddr.sin6_port == peeraddr.sin6_port &&
           memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr,
                  sizeof(localaddr.sin6_addr)) == 0;
  }
  return false;
}

} // namespace lynx::sockets
