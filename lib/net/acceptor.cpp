#include "lynx/net/acceptor.h"
#include "lynx/logger/logging.h"
#include "lynx/net/event_loop.h"
#include "lynx/net/inet_address.h"
#include "lynx/net/sockets_ops.h"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace lynx {

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop),
      accept_socket_(sockets::createNonblockingOrDie(listenAddr.family())),
      accept_channel_(loop, accept_socket_.fd()), listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(idle_fd_ >= 0);
  accept_socket_.setReuseAddr(true);
  accept_socket_.setReusePort(reuseport);
  accept_socket_.bindAddress(listenAddr);
  accept_channel_.setReadCallback([this](auto && /*PH1*/) { handleRead(); });
}

Acceptor::~Acceptor() {
  accept_channel_.disableAll();
  accept_channel_.remove();
  ::close(idle_fd_);
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  listening_ = true;
  accept_socket_.listen();
  accept_channel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peer_addr;
  // FIXME loop until no more
  int connfd = accept_socket_.accept(&peer_addr);
  if (connfd >= 0) {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (new_connection_callback_) {
      new_connection_callback_(connfd, peer_addr);
    } else {
      sockets::close(connfd);
    }
  } else {
    LOG_SYSERR << "in Acceptor::handleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of libev.
    if (errno == EMFILE) {
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_socket_.fd(), nullptr, nullptr);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

} // namespace lynx
