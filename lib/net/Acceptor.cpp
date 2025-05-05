#include "lynx/net/Acceptor.h"
#include "lynx/logger/Logging.h"
#include "lynx/net/EventLoop.h"
#include "lynx/net/InetAddress.h"

#include <cassert>
#include <fcntl.h>

namespace lynx {

namespace detail {

int createNonblockingOrDie() {
  int Sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (Sockfd < 0)
    LOG_SYSFATAL << "createNonblockingOrDie";

  return Sockfd;
}

} // namespace detail

Acceptor::Acceptor(EventLoop *Loop, const InetAddress &ListenAddr,
                   bool Reuseport)
    : Loop(Loop), AcceptSocket(detail::createNonblockingOrDie()),
      AcceptChannel(Loop, AcceptSocket.fd()), Listening(false),
      IdleFd(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(IdleFd >= 0);
  AcceptSocket.setReuseAddr(true);
  AcceptSocket.setReusePort(Reuseport);
  AcceptSocket.bindAddress(ListenAddr);
  AcceptChannel.setReadCallback([this](auto && /*PH1*/) { handleRead(); });
}

Acceptor::~Acceptor() {
  AcceptChannel.disableAll();
  AcceptChannel.remove();
  ::close(IdleFd);
}

void Acceptor::listen() {
  Loop->assertInLoopThread();
  Listening = true;
  AcceptSocket.listen();
  AcceptChannel.enableReading();
}

void Acceptor::handleRead() {
  Loop->assertInLoopThread();
  InetAddress PeerAddr;

  int Connfd = AcceptSocket.accept(&PeerAddr);
  if (Connfd >= 0) {
    if (NewConnectionCb) {
      NewConnectionCb(Connfd, PeerAddr);
    } else {
      if (::close(Connfd) < 0)
        LOG_SYSERR << "close";
    }
  } else {
    LOG_SYSERR << "in Acceptor::handleRead";
    if (errno == EMFILE) {
      ::close(IdleFd);
      IdleFd = ::accept(AcceptSocket.fd(), nullptr, nullptr);
      ::close(IdleFd);
      IdleFd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

} // namespace lynx
