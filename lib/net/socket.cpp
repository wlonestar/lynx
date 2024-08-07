#include "lynx/net/socket.h"
#include "lynx/logger/logging.h"
#include "lynx/net/inet_address.h"

#include <netinet/in.h>
#include <unistd.h>

namespace lynx {

Socket::~Socket() {
  if (::close(sockfd_) < 0) {
    LOG_SYSERR << "close";
  }
}

bool Socket::getTcpInfo(struct tcp_info *tcpi) const {
  socklen_t len = sizeof(*tcpi);
  memset(tcpi, 0, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char *buf, int len) const {
  struct tcp_info tcpi;
  bool ok = getTcpInfo(&tcpi);
  if (ok) {
    snprintf(buf, len,
             "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u",
             tcpi.tcpi_retransmits, tcpi.tcpi_rto, tcpi.tcpi_ato,
             tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss, tcpi.tcpi_lost,
             tcpi.tcpi_retrans, tcpi.tcpi_rtt, tcpi.tcpi_rttvar,
             tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);
  }
  return ok;
}

void Socket::bindAddress(const InetAddress &addr) {
  if (::bind(sockfd_,
             static_cast<const struct sockaddr *>(
                 static_cast<const void *>(addr.getSockAddr())),
             static_cast<socklen_t>(sizeof(struct sockaddr_in6))) < 0) {
    LOG_SYSFATAL << "bindOrDie";
  }
}

void Socket::listen() {
  if (::listen(sockfd_, SOMAXCONN) < 0) {
    LOG_SYSFATAL << "listenOrDie";
  }
}

int Socket::accept(InetAddress *peeraddr) {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  memset(&addr, 0, sizeof(addr));
  int connfd = ::accept4(
      sockfd_, static_cast<struct sockaddr *>(static_cast<void *>(&addr)), &len,
      SOCK_NONBLOCK | SOCK_CLOEXEC);
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
  } else {
    peeraddr->setSockAddr(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
    LOG_SYSERR << "shutdownWrite";
  }
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
               static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
               static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                         static_cast<socklen_t>(sizeof(optval)));
  if (ret < 0 && on) {
    LOG_SYSERR << "SO_REUSEPORT failed.";
  }
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
               static_cast<socklen_t>(sizeof(optval)));
}

} // namespace lynx
