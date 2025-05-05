#include "lynx/net/Socket.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/InetAddress.h"

#include <netinet/in.h>
#include <unistd.h>

namespace lynx {

Socket::~Socket() {
  if (::close(Sockfd) < 0)
    LOG_SYSERR << "close";
}

bool Socket::getTcpInfo(struct tcp_info *Tcpi) const {
  socklen_t Len = sizeof(*Tcpi);
  memset(Tcpi, 0, Len);
  return ::getsockopt(Sockfd, SOL_TCP, TCP_INFO, Tcpi, &Len) == 0;
}

bool Socket::getTcpInfoString(char *Buf, int Len) const {
  struct tcp_info Tcpi;
  bool Ok = getTcpInfo(&Tcpi);
  if (Ok) {
    snprintf(Buf, Len,
             "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u",
             Tcpi.tcpi_retransmits, Tcpi.tcpi_rto, Tcpi.tcpi_ato,
             Tcpi.tcpi_snd_mss, Tcpi.tcpi_rcv_mss, Tcpi.tcpi_lost,
             Tcpi.tcpi_retrans, Tcpi.tcpi_rtt, Tcpi.tcpi_rttvar,
             Tcpi.tcpi_snd_ssthresh, Tcpi.tcpi_snd_cwnd,
             Tcpi.tcpi_total_retrans);
  }
  return Ok;
}

void Socket::bindAddress(const InetAddress &Addr) {
  if (::bind(Sockfd,
             static_cast<const struct sockaddr *>(
                 static_cast<const void *>(Addr.getSockAddr())),
             static_cast<socklen_t>(sizeof(struct sockaddr_in6))) < 0)
    LOG_SYSFATAL << "bindOrDie";
}

void Socket::listen() {
  if (::listen(Sockfd, SOMAXCONN) < 0)
    LOG_SYSFATAL << "listenOrDie";
}

int Socket::accept(InetAddress *Peeraddr) {
  struct sockaddr_in Addr;
  socklen_t Len = sizeof(Addr);
  memset(&Addr, 0, sizeof(Addr));
  int Connfd = ::accept4(
      Sockfd, static_cast<struct sockaddr *>(static_cast<void *>(&Addr)), &Len,
      SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (Connfd < 0) {
    int SavedErrno = errno;
    LOG_SYSERR << "Socket::accept";
    switch (SavedErrno) {
    case EAGAIN:
    case ECONNABORTED:
    case EINTR:
    case EPROTO:
    case EPERM:
    case EMFILE:
      errno = SavedErrno;
      break;
    case EBADF:
    case EFAULT:
    case EINVAL:
    case ENFILE:
    case ENOBUFS:
    case ENOMEM:
    case ENOTSOCK:
    case EOPNOTSUPP:
      LOG_FATAL << "unexpected error of ::accept " << SavedErrno;
      break;
    default:
      LOG_FATAL << "unknown error of ::accept " << SavedErrno;
      break;
    }
  } else {
    Peeraddr->setSockAddr(Addr);
  }
  return Connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(Sockfd, SHUT_WR) < 0)
    LOG_SYSERR << "shutdownWrite";
}

void Socket::setTcpNoDelay(bool On) {
  int Optval = On ? 1 : 0;
  ::setsockopt(Sockfd, IPPROTO_TCP, TCP_NODELAY, &Optval,
               static_cast<socklen_t>(sizeof(Optval)));
}

void Socket::setReuseAddr(bool On) {
  int Optval = On ? 1 : 0;
  ::setsockopt(Sockfd, SOL_SOCKET, SO_REUSEADDR, &Optval,
               static_cast<socklen_t>(sizeof(Optval)));
}

void Socket::setReusePort(bool On) {
  int Optval = On ? 1 : 0;
  int Ret = ::setsockopt(Sockfd, SOL_SOCKET, SO_REUSEPORT, &Optval,
                         static_cast<socklen_t>(sizeof(Optval)));
  if (Ret < 0 && On)
    LOG_SYSERR << "SO_REUSEPORT failed.";
}

void Socket::setKeepAlive(bool On) {
  int Optval = On ? 1 : 0;
  ::setsockopt(Sockfd, SOL_SOCKET, SO_KEEPALIVE, &Optval,
               static_cast<socklen_t>(sizeof(Optval)));
}

} // namespace lynx
